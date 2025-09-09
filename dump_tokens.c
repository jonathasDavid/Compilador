#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "memmon.h"

// Usage: dump_tokens [file|-]
// helper to map token type to string
static const char *token_type_to_string(Token t) {
    switch (t.tipo) {
        case TOKEN_IDENTIFICADOR_VAR: return "IDENT_VAR";
        case TOKEN_IDENTIFICADOR_FUNC: return "IDENT_FUNC";
        case TOKEN_PALAVRA_RESERVADA: return "RESERVED";
        case TOKEN_NUMERO_INT: return "NUM_INT";
        case TOKEN_NUMERO_DEC: return "NUM_DEC";
        case TOKEN_OPERADOR: return "OPERATOR";
        case TOKEN_DELIMITADOR: return "DELIM";
        case TOKEN_TEXTO: return "TEXT";
        case TOKEN_COMENTARIO: return "COMMENT";
        case TOKEN_ERRO_LEXICO: return "LEX_ERROR";
        case TOKEN_FIM_DE_ARQUIVO: return "EOF_TOKEN";
        default: return "UNKNOWN";
    }
}

// small helper to print JSON-escaped string
static void print_json_string(const char *s) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        unsigned char c = *p;
        switch (c) {
            case '"': fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\b': fputs("\\b", stdout); break;
            case '\f': fputs("\\f", stdout); break;
            case '\n': fputs("\\n", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            default:
                if (c < 0x20) {
                    // control char, emit \u00XX
                    printf("\\u%04x", c);
                } else putchar(c);
        }
    }
    putchar('"');
}

// simple strdup replacement that uses mm_malloc
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)mm_malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

// free filters allocated with xstrdup
// free array of strings allocated with xstrdup
static void free_str_array(const char **arr, int count) {
    for (int i = 0; i < count; ++i) {
        if (arr[i]) mm_free((void*)arr[i]);
    }
}

int main(int argc, char **argv) {
    const char *program =
        "inteiro !a, !b2 = 7;\n"
        "escreva(\"Escreva um numero\", !b2);\n"
        "!a = !b2 + 3;\n"
        "se(!a <= !b2) escreva(\"A<=B\", !a);\n"
        "funcao __soma(!e, !aa) { inteiro !num; !num = !e + !aa; retorno !num; }\n"
        "";
    LexerState ls;
    char *stdin_buf = NULL; // if we read from stdin, keep to free later
    bool json_out = false;
    const char *input_path = NULL;
    size_t mem_limit_kb = 0; // 0 = no limit
    bool fail_on_alert = false;
    bool compact_json = false;
    const char *filter_spec = NULL;
    const char *filters[16];
    int filters_count = 0;
    const char *exclude_spec = NULL;
    const char *excludes[16];
    int excludes_count = 0;
    // parse flags in any order; first non-flag (or "-") is input path
    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "-j") == 0 || strcmp(a, "--json") == 0) { json_out = true; }
    else if (strcmp(a, "-c") == 0 || strcmp(a, "--compact") == 0) { compact_json = true; }
    else if (strcmp(a, "-f") == 0 || strcmp(a, "--fail-on-alert") == 0) { fail_on_alert = true; }
        else if (strcmp(a, "-F") == 0 || strcmp(a, "--filter") == 0) {
            if (i + 1 < argc) filter_spec = argv[++i];
        } else if (strncmp(a, "--filter=", 9) == 0) {
            filter_spec = a + 9;
        } else if (strcmp(a, "-E") == 0 || strcmp(a, "--exclude") == 0) {
            if (i + 1 < argc) exclude_spec = argv[++i];
        } else if (strncmp(a, "--exclude=", 10) == 0) {
            exclude_spec = a + 10;
        }
        else if (strcmp(a, "-m") == 0 || strcmp(a, "--memlimit") == 0) {
            if (i + 1 < argc) { mem_limit_kb = (size_t)atoi(argv[++i]); }
        } else if (strncmp(a, "-m", 2) == 0 && a[2] != '\0') {
            mem_limit_kb = (size_t)atoi(a + 2);
        } else if (strncmp(a, "--memlimit=", 11) == 0) {
            mem_limit_kb = (size_t)atoi(a + 11);
        } else if (!input_path) { input_path = a; }
    }
    // initialize memmon with configured limit (KB)
    memmon_init(mem_limit_kb);
    size_t mem_limit_bytes = mem_limit_kb * 1024UL;
    int token_check_counter = 0;
    int alerta_printed = 0;

    if (input_path) {
        if (strcmp(input_path, "-") == 0) {
            // read all stdin but check memory before expanding the buffer
            size_t cap = 1024; size_t len = 0;
            const size_t safety_margin = 128;
            if (mem_limit_bytes > 0) {
                size_t cur = memmon_current_bytes();
                size_t projected = cur + (cap + 1) + safety_margin;
                if (projected >= mem_limit_bytes) {
                    fprintf(stderr, "ERRO: Alocacao inicial prevista %lu bytes >= limite %lu bytes\n", (unsigned long)projected, (unsigned long)mem_limit_bytes);
                    if (fail_on_alert) {
                        fprintf(stderr, "ERRO: fail-on-alert ativado; abortando antes da alocacao inicial\n");
                        memmon_report_peak();
                        free_str_array(filters, filters_count);
                        return 3;
                    }
                    memmon_report_peak();
                    free_str_array(filters, filters_count);
                    return 2;
                }
            }
            stdin_buf = (char*)mm_malloc(cap+1);
            if (!stdin_buf) { fprintf(stderr, "Erro: sem memoria ao ler stdin\n"); free_str_array(filters, filters_count); return 1; }
            int c;
            // safety_margin already defined later; reuse same value
            while ((c = fgetc(stdin)) != EOF) {
                if (len + 1 >= cap) {
                    size_t new_cap = cap * 2;
                    size_t delta = (new_cap + 1) - (cap + 1); // additional bytes we will request
                    if (mem_limit_bytes > 0) {
                        size_t cur = memmon_current_bytes();
                        size_t projected = cur + delta + safety_margin;
                        double pct = (projected * 100.0) / (double)mem_limit_bytes;
                        if (pct >= 100.0) {
                            fprintf(stderr, "ERRO: Memoria Insuficiente prevista: proj %lu bytes >= limite %lu bytes\n", (unsigned long)projected, (unsigned long)mem_limit_bytes);
                            memmon_report_peak();
                            mm_free(stdin_buf);
                            free_str_array(filters, filters_count);
                            return 2;
                        }
                        if (pct >= 90.0) {
                            fprintf(stderr, "ALERTA: Expansao projetada de memoria em %.1f%% do limite (%lu/%lu bytes)\n", pct, (unsigned long)projected, (unsigned long)mem_limit_bytes);
                            if (fail_on_alert) {
                                fprintf(stderr, "ERRO: fail-on-alert ativado; abortando antes de realloc\n");
                                memmon_report_peak();
                                mm_free(stdin_buf);
                                free_str_array(filters, filters_count);
                                return 3;
                            }
                        }
                    }
                    cap = new_cap;
                    char *n = (char*)mm_realloc(stdin_buf, cap+1);
                    if (!n) { mm_free(stdin_buf); fprintf(stderr, "Erro: sem memoria\n"); return 1; }
                    stdin_buf = n;
                }
                stdin_buf[len++] = (char)c;
            }
            stdin_buf[len] = '\0';
            // initialize lexer with our buffer; mark owns_buffer = 0 so lexer_dispose won't free it (we'll free)
            inicializar_lexer(&ls, stdin_buf);
            ls.owns_buffer = 0;
        } else {
            lexer_init_from_file(&ls, input_path);
            if (!ls.codigo_fonte) { fprintf(stderr, "Erro: nao foi possivel abrir arquivo '%s'\n", input_path); free_str_array(filters, filters_count); return 1; }
        }
    } else {
        inicializar_lexer(&ls, program);
    }
    bool first = true;
    if (json_out) {
        if (compact_json) printf("["); else printf("[");
    }
    // parse filters
    if (filter_spec) {
        // split by comma
        char *tmp = xstrdup(filter_spec);
        char *p = tmp;
        while (p && *p && filters_count < 16) {
            char *comma = strchr(p, ',');
            if (comma) { *comma = '\0'; filters[filters_count++] = xstrdup(p); p = comma + 1; }
            else { filters[filters_count++] = xstrdup(p); break; }
        }
        if (tmp) mm_free(tmp);
    }
    if (exclude_spec) {
        char *tmp2 = xstrdup(exclude_spec);
        char *q = tmp2;
        while (q && *q && excludes_count < 16) {
            char *comma = strchr(q, ',');
            if (comma) { *comma = '\0'; excludes[excludes_count++] = xstrdup(q); q = comma + 1; }
            else { excludes[excludes_count++] = xstrdup(q); break; }
        }
        if (tmp2) mm_free(tmp2);
    }
    while (1) {
        Token t = obter_proximo_token(&ls);
        if (t.tipo == TOKEN_FIM_DE_ARQUIVO) {
            if (!json_out) printf("EOF\n");
            break;
        }
        // periodic memory usage check every 16 tokens
        if (mem_limit_bytes > 0) {
            token_check_counter++;
            if ((token_check_counter & 15) == 0) {
                size_t cur = memmon_current_bytes();
                double pct = (cur * 100.0) / (double)mem_limit_bytes;
                        if (pct >= 100.0) {
                    // fail fast
                    fprintf(stderr, "ERRO: Memoria Insuficiente: uso %lu bytes >= limite %lu bytes\n", (unsigned long)cur, (unsigned long)mem_limit_bytes);
                    memmon_report_peak();
                    lexer_dispose(&ls);
                    if (stdin_buf) mm_free(stdin_buf);
                    free_str_array(filters, filters_count);
                    return 2;
                } else if (pct >= 90.0 && !alerta_printed) {
                    fprintf(stderr, "ALERTA: Uso de memoria em %.1f%% do limite (%lu/%lu bytes)\n", pct, (unsigned long)cur, (unsigned long)mem_limit_bytes);
                    alerta_printed = 1;
                    if (fail_on_alert) {
                        fprintf(stderr, "ERRO: fail-on-alert ativado; abortando por alerta de memoria\n");
                        memmon_report_peak();
                    lexer_dispose(&ls);
                    if (stdin_buf) mm_free(stdin_buf);
                    free_str_array(filters, filters_count);
                    return 3;
                    }
                }
            }
        }
        const char *tipo_str = token_type_to_string(t);
        // filter by type if requested
        if (filters_count > 0) {
            int matched = 0;
            for (int fi = 0; fi < filters_count; ++fi) {
                if (strcmp(filters[fi], tipo_str) == 0) { matched = 1; break; }
            }
            if (!matched) continue;
        }
        // exclusion check
        if (excludes_count > 0) {
            int excl = 0;
            for (int ei = 0; ei < excludes_count; ++ei) {
                if (strcmp(excludes[ei], tipo_str) == 0) { excl = 1; break; }
            }
            if (excl) continue;
        }
        if (json_out) {
            if (!first) {
                if (compact_json) printf(","); else printf(",\n");
            }
            first = false;
            if (!compact_json) printf("  ");
            printf("{\"type\":\""); fputs(tipo_str, stdout); printf("\",\"lexeme\":"); print_json_string(t.lexema); printf(",\"line\":%d}", t.linha);
        } else {
            printf("Token: tipo=%s lexema='%s' linha=%d\n", tipo_str, t.lexema, t.linha);
        }
    }
    if (json_out) {
        if (compact_json) printf("]\n"); else printf("\n]\n");
    }
    lexer_dispose(&ls);
    if (stdin_buf) mm_free(stdin_buf);
    // report memmon peak usage
    memmon_report_peak();
    free_str_array(filters, filters_count);
    return 0;
}
