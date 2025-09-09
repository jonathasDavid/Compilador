// New clean implementation
#include "parser.h"
#include "symbol_table.h"
#include "semantic.h"
#include "lexer.h"
#include "memmon.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static LexerState *ls_global = NULL;
static Token lookahead;

/* parser tunables */
#define MAX_IO_ARGS 16

static void next_token(void) {
    lookahead = obter_proximo_token(ls_global);
}

// ...peek functionality removed (not used) to silence unused-function warning

static Token *cur(void) { return &lookahead; }

// expression info carries inferred type and size/decimal parts
typedef struct { VarType type; int size; int dec_before; int dec_after; } ExprInfo;

// forward declarations
static ExprInfo parse_expression(void);
static void parse_statement(void);
static void parse_function(VarType declared_ret, int declared_size, int declared_dec_before, int declared_dec_after);

void parser_init_from_lexer(LexerState *ls) {
    ls_global = ls;
    symtab_init();
    semantic_init();
    // initialize memory monitor with default 2048 KB limit
    memmon_init(2048);
    next_token();
}

static void expect(TokenType t, const char *msg) {
    Token *p = cur();
    if (!p || p->tipo != t) {
        int line = p ? p->linha : -1;
        fprintf(stderr, "ERRO SINTATICO: linha %d: %s\n", line, msg);
        exit(1);
    }
}

static int is_op(const char *s) {
    Token *t = cur();
    return t && t->tipo == TOKEN_OPERADOR && strcmp(t->lexema, s) == 0;
}
static int is_delim(const char *s) {
    Token *t = cur();
    return t && t->tipo == TOKEN_DELIMITADOR && strcmp(t->lexema, s) == 0;
}

// parse declarations like: inteiro !a, !b = 7; and texto[10] !s;
static void parse_declaration(void) {
    Token *t = cur();
    char typelex[32];
    strncpy(typelex, t->lexema, sizeof(typelex)-1);
    typelex[sizeof(typelex)-1] = '\0';
    // consume type
    next_token();
    int declared_size = 0;
    int declared_dec_before = 0;
    int declared_dec_after = 0;
    // if type is texto or decimal and next token is '[', parse size
    if ((strcmp(typelex, "texto") == 0 || strcmp(typelex, "decimal") == 0) && cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "[") == 0) {
        // consume '['
        if (strcmp(typelex, "decimal") == 0) {
            /* debug suppressed */
        }
        next_token();
    Token *numtok = cur();
    /* debug suppressed */
    if (!numtok) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero dentro de '[' ']'\n", -1); exit(1); }
        // for texto: single integer; for decimal: accept TOKEN_NUMERO_DEC ("A.B") or int '.' int sequence
        if (strcmp(typelex, "texto") == 0) {
            if (numtok->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero inteiro dentro de '[' ']'\n", numtok?numtok->linha:-1); exit(1); }
            declared_size = atoi(numtok->lexema);
            if (declared_size <= 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: tamanho invalido '%s' em colchetes\n", numtok->linha, numtok->lexema); exit(1); }
            next_token();
            if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho\n", cur()?cur()->linha:-1); exit(1); }
            next_token();
        } else { // decimal expects A.B or TOKEN_NUMERO_DEC
            if (numtok->tipo == TOKEN_NUMERO_DEC) {
                char buf[64]; strncpy(buf, numtok->lexema, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
                char *dot = strchr(buf, '.');
                if (!dot) { fprintf(stderr, "ERRO SINTATICO: linha %d: formato decimal invalido '%s'\n", numtok->linha, numtok->lexema); exit(1); }
                *dot = '\0';
                declared_dec_before = atoi(buf);
                declared_dec_after = atoi(dot+1);
                if (declared_dec_before <= 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: parte inteira invalida '%s'\n", numtok->linha, numtok->lexema); exit(1); }
                if (declared_dec_after < 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: parte fracionaria invalida '%s'\n", numtok->linha, numtok->lexema); exit(1); }
                next_token();
                if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho\n", cur()?cur()->linha:-1); exit(1); }
                next_token();
            } else {
                if (numtok->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero inteiro dentro de '[' ']'\n", numtok?numtok->linha:-1); exit(1); }
                declared_dec_before = atoi(numtok->lexema);
                if (declared_dec_before <= 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: parte inteira invalida '%s'\n", numtok->linha, numtok->lexema); exit(1); }
                next_token();
                if (!(cur()->tipo == TOKEN_OPERADOR && strcmp(cur()->lexema, ".") == 0)) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '.' em decimal declaration\n", cur()?cur()->linha:-1); exit(1); }
                next_token();
                Token *numtok2 = cur();
                if (!numtok2 || numtok2->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero inteiro apos '.' em decimal declaration\n", numtok2?numtok2->linha:-1); exit(1); }
                declared_dec_after = atoi(numtok2->lexema);
                if (declared_dec_after < 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: parte fracionaria invalida '%s'\n", numtok2->linha, numtok2->lexema); exit(1); }
                next_token();
                if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho\n", cur()?cur()->linha:-1); exit(1); }
                next_token();
            }
        }
    }
    while (1) {
        Token *id = cur();
        if (!id || id->tipo != TOKEN_IDENTIFICADOR_VAR) { expect(TOKEN_IDENTIFICADOR_VAR, "Esperado identificador de variavel"); }
        char name[64]; strncpy(name, id->lexema, sizeof(name)-1); name[sizeof(name)-1] = '\0';
    VarType vt = TYPE_UNKNOWN;
        if (strcmp(typelex, "inteiro") == 0) vt = TYPE_INT;
        else if (strcmp(typelex, "texto") == 0) vt = TYPE_TEXT;
        else if (strcmp(typelex, "decimal") == 0) vt = TYPE_DECIMAL;
    semantic_check_declaration(name, vt, id->linha, declared_size, declared_dec_before, declared_dec_after);
        next_token();
        // optional assignment
        if (is_op("=")) {
            next_token();
            ExprInfo r = parse_expression();
            semantic_check_assignment(name, r.type, id->linha, r.size, r.dec_after);
                    // record last known value size / decimal parts for the variable if it's a texto/decimal
                    symtab_set_last_value_size(name, r.size);
                    symtab_set_last_decimal_parts(name, r.dec_before, r.dec_after);
        }
        if (is_delim(",")) { next_token(); continue; }
        break;
    }
    if (!is_delim(";")) { fprintf(stderr, "ERRO SINTATICO: linha %d: Esperado ';' no final da declaracao\n", cur()?cur()->linha:-1); exit(1); }
    next_token();
}

// Expression parser
static ExprInfo parse_primary(void) {
    Token *t = cur();
    if (!t) { fprintf(stderr, "ERRO SINTATICO: expressao incompleta\n"); exit(1); }
    ExprInfo r = { .type = TYPE_UNKNOWN, .size = 0, .dec_before = 0, .dec_after = 0 };
    if (t->tipo == TOKEN_NUMERO_INT) { next_token(); r.type = TYPE_INT; return r; }
    if (t->tipo == TOKEN_NUMERO_DEC) {
        // lexeme contains form digits.digits
        char buf[128]; strncpy(buf, t->lexema, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
        char *dot = strchr(buf, '.');
        if (dot) {
            *dot = '\0';
            char *intpart = buf; char *fracpart = dot+1;
            // use digit counts for decimal literal parts, not numeric values
            r.dec_before = (int)strlen(intpart);
            r.dec_after = (int)strlen(fracpart);
            r.size = r.dec_before; // for passing to semantic checks
        }
        next_token(); r.type = TYPE_DECIMAL; return r; }
    if (t->tipo == TOKEN_TEXTO) { r.type = TYPE_TEXT; r.size = (int)strlen(t->lexema); next_token(); return r; }
    if (t->tipo == TOKEN_IDENTIFICADOR_VAR) {
        Symbol *s = symtab_lookup_any(t->lexema);
        if (!s) { fprintf(stderr, "ERRO: linha %d: Variavel '%s' nao declarada\n", t->linha, t->lexema); exit(1); }
        r.type = s->type;
        // prefer declared sizes, but if not declared use last assigned sizes
        if (s->size > 0) r.size = s->size; else r.size = s->last_value_size;
    if (s->dec_before > 0) r.dec_before = s->dec_before; else r.dec_before = s->last_dec_before;
    if (s->dec_after > 0) r.dec_after = s->dec_after; else r.dec_after = s->last_dec_after;
    if (r.type == TYPE_DECIMAL) r.size = r.dec_before;
        next_token();
        return r;
    }
    if (t->tipo == TOKEN_DELIMITADOR && strcmp(t->lexema, "(") == 0) {
        next_token(); ExprInfo v = parse_expression(); if (!is_delim(")")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ')'\n", cur()?cur()->linha:-1); exit(1);} next_token(); return v;
    }
    fprintf(stderr, "ERRO SINTATICO: linha %d: token invalido em expressao '%s'\n", t->linha, t->lexema);
    exit(1);
}

static VarType promote(VarType a, VarType b) {
    if (a == TYPE_DECIMAL || b == TYPE_DECIMAL) return TYPE_DECIMAL;
    if (a == TYPE_TEXT || b == TYPE_TEXT) return TYPE_TEXT;
    if (a == TYPE_INT && b == TYPE_INT) return TYPE_INT;
    return TYPE_UNKNOWN;
}

static ExprInfo parse_unary(void) {
    Token *t = cur();
    if (t && t->tipo == TOKEN_OPERADOR && (strcmp(t->lexema, "+") == 0 || strcmp(t->lexema, "-") == 0)) { next_token(); return parse_unary(); }
    return parse_primary();
}

static ExprInfo parse_mul(void) {
    ExprInfo left = parse_unary();
    while (is_op("*") || is_op("/")) {
        next_token(); ExprInfo right = parse_unary();
        left.type = promote(left.type, right.type);
        if (left.type == TYPE_TEXT) left.size = (left.size>right.size?left.size:right.size);
        else if (left.type == TYPE_DECIMAL) {
            left.dec_before = (left.dec_before>right.dec_before?left.dec_before:right.dec_before);
            left.dec_after = (left.dec_after>right.dec_after?left.dec_after:right.dec_after);
        } else left.size = 0;
    }
    return left;
}

static ExprInfo parse_add(void) {
    ExprInfo left = parse_mul();
    while (is_op("+") || is_op("-")) {
        next_token(); ExprInfo right = parse_mul();
        left.type = promote(left.type, right.type);
        if (left.type == TYPE_TEXT) left.size = (left.size>right.size?left.size:right.size);
        else if (left.type == TYPE_DECIMAL) {
            left.dec_before = (left.dec_before>right.dec_before?left.dec_before:right.dec_before);
            left.dec_after = (left.dec_after>right.dec_after?left.dec_after:right.dec_after);
        } else left.size = 0;
    }
    return left;
}

static ExprInfo parse_relational(void) {
    ExprInfo left = parse_add();
    while (is_op("<") || is_op(">") || is_op("<=") || is_op(">=") || is_op("==") || is_op("<>")) {
        Token *op = cur(); next_token(); ExprInfo right = parse_add(); if (left.type != right.type && left.type != TYPE_UNKNOWN && right.type != TYPE_UNKNOWN) fprintf(stderr, "ALERTA SEMANTICO: linha %d: comparacao entre tipos diferentes\n", op->linha); left.type = TYPE_INT; left.size = 0;
    }
    return left;
}

static ExprInfo parse_equality(void) { return parse_relational(); }
static ExprInfo parse_logical_and(void) { ExprInfo left = parse_equality(); while (is_op("&&")) { next_token(); parse_equality(); left.type = TYPE_INT; left.size = 0; } return left; }
static ExprInfo parse_logical_or(void) { ExprInfo left = parse_logical_and(); while (is_op("||")) { next_token(); parse_logical_and(); left.type = TYPE_INT; left.size = 0; } return left; }
static ExprInfo parse_expression(void) { return parse_logical_or(); }

static void parse_statement(void);

static void parse_if(void) {
    Token *t = cur(); int line = t->linha; next_token(); if (!is_delim("(")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '(' apos 'se'\n", line); exit(1); } next_token(); parse_expression(); if (!is_delim(")")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ')' apos condicao\n", line); exit(1); } next_token();
    if (cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "{") == 0) { next_token(); while (!(cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "}") == 0)) parse_statement(); next_token(); } else parse_statement();
    if (cur()->tipo == TOKEN_PALAVRA_RESERVADA && strcmp(cur()->lexema, "senao") == 0) { next_token(); if (cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "{") == 0) { next_token(); while (!(cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "}") == 0)) parse_statement(); next_token(); } else parse_statement(); }
}

static void parse_return(void) { Token *t = cur(); int line = t->linha; next_token(); ExprInfo r = parse_expression(); if (!is_delim(";")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ';' apos retorno\n", cur()?cur()->linha:-1); exit(1); } next_token(); semantic_check_return(r.type, r.size, r.dec_after, line); }

static void parse_function(VarType declared_ret, int declared_size, int declared_dec_before, int declared_dec_after) {
    Token *t = cur();
    int line = t ? t->linha : -1;
    // Accept two entry conventions:
    // 1) cur() == 'funcao' (caller didn't consume it) -> consume it and expect name
    // 2) cur() == function name (caller already consumed 'funcao') -> use it directly
    if (t && t->tipo == TOKEN_PALAVRA_RESERVADA && strcmp(t->lexema, "funcao") == 0) {
        // consume 'funcao'
        next_token();
        t = cur();
    }
    Token *name = cur();
    if (!name || name->tipo != TOKEN_IDENTIFICADOR_FUNC) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado nome de funcao com '__'\n", line); exit(1); }
    char fname[64]; strncpy(fname, name->lexema, sizeof(fname)-1); fname[sizeof(fname)-1] = '\0';
    // declare function and record decimal parts if provided
    semantic_declare_function(fname, declared_ret, name->linha, declared_size);
    if (declared_ret == TYPE_DECIMAL) {
        Symbol *f = symtab_lookup_any(fname);
        if (f) { f->dec_before = declared_dec_before; f->dec_after = declared_dec_after; }
    }
    // consume function name
    next_token();
    if (!is_delim("(")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '(' apos nome de funcao\n", line); exit(1); }
    // consume '('
    next_token();
    // parse parameters: each parameter MUST be <tipo> [n]? <!identificador>
    char params[MAX_IO_ARGS][64]; int param_lines[MAX_IO_ARGS]; VarType param_types[MAX_IO_ARGS]; int param_sizes[MAX_IO_ARGS]; int param_dec_before[MAX_IO_ARGS]; int param_dec_after[MAX_IO_ARGS]; int param_count = 0;
    if (!is_delim(")")) {
        while (1) {
            Token *ptype_tok = cur();
            if (!ptype_tok || ptype_tok->tipo != TOKEN_PALAVRA_RESERVADA) {
                fprintf(stderr, "ERRO SINTATICO: linha %d: parametro de funcao deve iniciar com o tipo (inteiro/texto/decimal)\n", ptype_tok?ptype_tok->linha:-1);
                exit(1);
            }
            VarType ptype = TYPE_UNKNOWN;
            int psize = 0; int pdec_before = 0; int pdec_after = 0;
            if (strcmp(ptype_tok->lexema, "inteiro") == 0) ptype = TYPE_INT;
            else if (strcmp(ptype_tok->lexema, "texto") == 0) ptype = TYPE_TEXT;
            else if (strcmp(ptype_tok->lexema, "decimal") == 0) ptype = TYPE_DECIMAL;
            else { fprintf(stderr, "ERRO SINTATICO: linha %d: tipo de parametro invalido '%s'\n", ptype_tok->linha, ptype_tok->lexema); exit(1); }
            // consume type
            next_token();
            // optional [n] for texto/decimal
            if ((ptype == TYPE_TEXT || ptype == TYPE_DECIMAL) && cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "[") == 0) {
                next_token(); // consume '['
                Token *numtok = cur();
                if (!numtok || numtok->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: parametro texto/decimal precisa de tamanho inteiro dentro de '[' ']'\n", numtok?numtok->linha:-1); exit(1); }
                if (ptype == TYPE_TEXT) {
                    psize = atoi(numtok->lexema);
                    if (psize <= 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: tamanho invalido '%s' para parametro\n", numtok->linha, numtok->lexema); exit(1); }
                    next_token();
                    if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho do parametro\n", cur()?cur()->linha:-1); exit(1); }
                    next_token();
                } else { // decimal param expects A.B
                    pdec_before = atoi(numtok->lexema);
                    next_token();
                    if (!(cur()->tipo == TOKEN_OPERADOR && strcmp(cur()->lexema, ".") == 0)) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '.' em parametro decimal\n", cur()?cur()->linha:-1); exit(1); }
                    next_token();
                    Token *numtok2 = cur();
                    if (!numtok2 || numtok2->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero apos '.' em parametro decimal\n", numtok2?numtok2->linha:-1); exit(1); }
                    pdec_after = atoi(numtok2->lexema);
                    next_token();
                    if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho do parametro decimal\n", cur()?cur()->linha:-1); exit(1); }
                    next_token();
                }
            }
            Token *p = cur();
            if (!p || p->tipo != TOKEN_IDENTIFICADOR_VAR) { fprintf(stderr, "ERRO SINTATICO: linha %d: parametro de funcao esperado como '!nome'\n", p? p->linha : -1); exit(1); }
            strncpy(params[param_count], p->lexema, sizeof(params[param_count])-1); params[param_count][sizeof(params[param_count])-1] = '\0';
            param_lines[param_count] = p->linha;
            param_types[param_count] = ptype;
            param_sizes[param_count] = psize;
            param_dec_before[param_count] = pdec_before;
            param_dec_after[param_count] = pdec_after;
            param_count++;
            next_token();
            if (is_delim(",")) { next_token(); continue; }
            break;
        }
    }
    // expect and consume ')'
    if (!is_delim(")")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ')' apos parametros de funcao\n", cur()?cur()->linha:-1); exit(1); }
    next_token();
    if (!is_delim("{")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '{' inicio da funcao\n", line); exit(1); }
    next_token();
    // enter function scope and register parameters as local symbols
    semantic_enter_function(fname);
    for (int i=0;i<param_count;i++) {
        VarType pt = param_types[i];
        int psize = param_sizes[i];
        int pbefore = param_dec_before[i];
        int pafter = param_dec_after[i];
        if (!symtab_insert(params[i], pt, param_lines[i], psize, pbefore, pafter)) {
            fprintf(stderr, "ERRO: linha %d: nao foi possivel inserir parametro '%s'\n", param_lines[i], params[i]); exit(1);
        }
    }
    while (!(cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "}") == 0)) { if (cur()->tipo == TOKEN_PALAVRA_RESERVADA && strcmp(cur()->lexema, "retorno") == 0) { parse_return(); continue; } parse_statement(); }
    next_token(); semantic_leave_function();
}

static void parse_io(int is_leia) {
    Token *t = cur();
    int line = t ? t->linha : -1;
    next_token();
    if (!is_delim("(")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '(' apos comando\n", line); exit(1); }
    next_token();

    int literal_lengths[MAX_IO_ARGS]; int lit_count = 0;
    int dec_lit_before[MAX_IO_ARGS]; int dec_lit_after[MAX_IO_ARGS]; int dec_lit_count = 0;
    char var_names[MAX_IO_ARGS][64]; int var_sizes[MAX_IO_ARGS]; int var_dec_before[MAX_IO_ARGS]; int var_dec_after[MAX_IO_ARGS]; int var_count = 0;

    while (1) {
        Token *a = cur();
        if (!a) { fprintf(stderr, "ERRO SINTATICO: linha %d: argumentos invalidos\n", line); exit(1); }
        if (is_leia) {
            if (a->tipo != TOKEN_IDENTIFICADOR_VAR) { fprintf(stderr, "ERRO SINTATICO: linha %d: leia espera variavel\n", a->linha); exit(1); }
            if (!symtab_lookup_any(a->lexema)) { fprintf(stderr, "ERRO: linha %d: Variavel '%s' nao declarada\n", a->linha, a->lexema); exit(1); }
        } else {
            if (!(a->tipo == TOKEN_IDENTIFICADOR_VAR || a->tipo == TOKEN_TEXTO || a->tipo == TOKEN_NUMERO_DEC)) { fprintf(stderr, "ERRO SINTATICO: linha %d: escreva espera texto, decimal literal ou variavel\n", a->linha); exit(1); }
            if (a->tipo == TOKEN_TEXTO) {
                int L = (int)strlen(a->lexema);
                if (lit_count < 16) literal_lengths[lit_count++] = L;
            }
            if (a->tipo == TOKEN_NUMERO_DEC) {
                char tmpd[64]; strncpy(tmpd, a->lexema, sizeof(tmpd)-1); tmpd[sizeof(tmpd)-1] = '\0'; char *d = strchr(tmpd, '.');
                if (d) { *d='\0'; int lb = (int)strlen(tmpd); int la = (int)strlen(d+1); if (dec_lit_count < 16) { dec_lit_before[dec_lit_count] = lb; dec_lit_after[dec_lit_count] = la; dec_lit_count++; } }
            }
            if (a->tipo == TOKEN_IDENTIFICADOR_VAR) {
                Symbol *s = symtab_lookup_any(a->lexema);
                if (!s) { fprintf(stderr, "ERRO: linha %d: Variavel '%s' nao declarada\n", a->linha, a->lexema); exit(1); }
                strncpy(var_names[var_count], a->lexema, sizeof(var_names[var_count])-1);
                var_names[var_count][sizeof(var_names[var_count])-1] = '\0';
                if (s->type == TYPE_TEXT) { var_sizes[var_count] = s->size; var_dec_before[var_count]=0; var_dec_after[var_count]=0; }
                else if (s->type == TYPE_DECIMAL) { var_sizes[var_count] = 0; var_dec_before[var_count] = s->dec_before; var_dec_after[var_count] = s->dec_after; }
                else { var_sizes[var_count] = 0; var_dec_before[var_count]=0; var_dec_after[var_count]=0; }
                var_count++;
            }
        }
        next_token();
        if (is_delim(",")) { next_token(); continue; }
        break;
    }

    if (!is_delim(")")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ')'\n", cur()?cur()->linha:-1); exit(1); }
    next_token();

    // A1: warn when a literal passed to escreva is larger than any variable passed in the same call
    if (!is_leia && var_count > 0) {
        // text literal checks
        if (lit_count > 0) {
            for (int i = 0; i < lit_count; ++i) {
                int L = literal_lengths[i];
                for (int j = 0; j < var_count; ++j) {
                    int S = var_sizes[j];
                    if (S > 0 && L > S) {
                        fprintf(stderr, "ALERTA SEMANTICO: linha %d: literal de tamanho %d excede tamanho declarado da variavel '%s' (decl: %d, literal: %d)\n", line, L, var_names[j], S, L);
                    }
                }
            }
        }
        // decimal literal checks: compare each decimal literal's digits against decimal variables in call
        if (dec_lit_count > 0) {
            for (int i = 0; i < dec_lit_count; ++i) {
                int lb = dec_lit_before[i]; int la = dec_lit_after[i];
                for (int j = 0; j < var_count; ++j) {
                    // if variable is decimal, compare
                    if (var_dec_before[j] > 0) {
                        if (lb > var_dec_before[j]) {
                            fprintf(stderr, "ALERTA SEMANTICO: linha %d: literal decimal parte inteira (%d) excede parte inteira da variavel '%s' (decl: %d, lit: %d)\n", line, lb, var_names[j], var_dec_before[j], lb);
                        }
                    }
                    if (var_dec_after[j] > 0) {
                        if (la > var_dec_after[j]) {
                            fprintf(stderr, "ALERTA SEMANTICO: linha %d: literal decimal parte fracionaria (%d) excede parte fracionaria da variavel '%s' (decl: %d, lit: %d)\n", line, la, var_names[j], var_dec_after[j], la);
                        }
                    }
                }
            }
        }
    }
        // A3: additionally warn if any variable's last assigned value is larger than its declared size
        if (!is_leia && var_count > 0) {
            for (int j = 0; j < var_count; ++j) {
                int last = symtab_get_last_value_size(var_names[j]);
                int decl = var_sizes[j];
                if (decl > 0 && last > decl) {
                    fprintf(stderr, "ALERTA SEMANTICO: linha %d: variavel '%s' contem ultimo valor de tamanho %d que excede seu tamanho declarado %d\n", line, var_names[j], last, decl);
                }
            }
        }

    if (!is_delim(";")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ';' apos comando\n", cur()?cur()->linha:-1); exit(1); }
    next_token();
}

static void parse_for(void) {
    Token *t = cur();
    int line = t ? t->linha : -1;
    /* consume 'para' */
    next_token();
    if (!is_delim("(")) {
        fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '(' apos 'para'\n", line);
        exit(1);
    }
    /* consume '(' */
    next_token();

    /* init expression (consume until first ';') */
    while (!is_delim(";")) next_token();
    /* consume ';' */
    next_token();

    /* condition expression (consume until second ';') */
    while (!is_delim(";")) next_token();
    /* consume ';' */
    next_token();

    /* post expression (consume until ')') */
    while (!is_delim(")")) next_token();
    /* consume ')' */
    next_token();

    if (cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "{") == 0) {
        next_token();
        while (!(cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "}") == 0)) parse_statement();
        next_token();
    } else {
        parse_statement();
    }
}

static void parse_assignment(void) {
    Token *id = cur();
    if (!id || id->tipo != TOKEN_IDENTIFICADOR_VAR) expect(TOKEN_IDENTIFICADOR_VAR, "Esperado identificador no inicio de atribuicao");
    
    char name[64]; strncpy(name, id->lexema, sizeof(name)-1); name[sizeof(name)-1]='\0';
    next_token();
    if (!is_op("=")) expect(TOKEN_OPERADOR, "Esperado '=' para atribuicao");
    next_token();
    ExprInfo r = parse_expression();
    if (!is_delim(";")) { fprintf(stderr, "ERRO SINTATICO: linha %d: Esperado ';' no final da atribuicao\\n", cur()?cur()->linha:-1); exit(1); }
    next_token();
    
    semantic_check_assignment(name, r.type, id->linha, r.size, r.dec_after);
    // store last known decimal parts for variable
    symtab_set_last_decimal_parts(name, r.dec_before, r.dec_after);
}

static void parse_statement(void) {
    Token *t = cur(); if (!t) return; if (t->tipo == TOKEN_PALAVRA_RESERVADA) {
        if (strcmp(t->lexema, "se") == 0) { parse_if(); return; }
        if (strcmp(t->lexema, "leia") == 0) { parse_io(1); return; }
        if (strcmp(t->lexema, "escreva") == 0) { parse_io(0); return; }
        if (strcmp(t->lexema, "para") == 0) { parse_for(); return; }
    if (strcmp(t->lexema, "funcao") == 0) { parse_function(TYPE_UNKNOWN, 0, 0, 0); return; }
        if (strcmp(t->lexema, "principal") == 0) {
            // parse principal() { ... } as the program entry point (no parameters allowed)
            Token *tt = cur(); int line = tt ? tt->linha : -1;
            next_token(); // consume 'principal'
            if (!is_delim("(")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '(' apos 'principal'\n", line); exit(1); }
            next_token();
            if (!is_delim(")")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ')' apos 'principal'\n", line); exit(1); }
            next_token();
            if (!is_delim("{")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado '{' inicio do principal\n", line); exit(1); }
            // declare principal as a function in symbol table
            semantic_declare_function("principal", TYPE_UNKNOWN, line, 0);
            next_token();
            semantic_enter_function("principal");
            while (!(cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "}") == 0)) {
                if (cur()->tipo == TOKEN_PALAVRA_RESERVADA && strcmp(cur()->lexema, "retorno") == 0) { parse_return(); continue; }
                parse_statement();
            }
            next_token(); // consume '}'
            semantic_leave_function();
            return;
        }
        if (strcmp(t->lexema, "inteiro") == 0 || strcmp(t->lexema, "texto") == 0 || strcmp(t->lexema, "decimal") == 0) {
            // possible typed function declaration: <tipo> funcao __name(...) or a variable declaration
            VarType declared = TYPE_UNKNOWN;
            if (strcmp(t->lexema, "inteiro") == 0) declared = TYPE_INT;
            else if (strcmp(t->lexema, "texto") == 0) declared = TYPE_TEXT;
            else if (strcmp(t->lexema, "decimal") == 0) declared = TYPE_DECIMAL;
            // multi-token lookahead using a lexer state copy to detect optional [n] then 'funcao'
            int declared_size = 0;
            int declared_dec_before = 0; int declared_dec_after = 0;
            LexerState copy = *ls_global; // copy lexer position after the current type token
            Token la1 = obter_proximo_token(&copy); // token immediately after the type
            int is_func_decl = 0;
            if (la1.tipo == TOKEN_DELIMITADOR && strcmp(la1.lexema, "[") == 0) {
                Token la2 = obter_proximo_token(&copy);
                // If decimal type, la2 may be TOKEN_NUMERO_DEC (A.B) or NUMERO_INT followed by '.' NUMERO_INT
                if (la2.tipo == TOKEN_NUMERO_DEC) {
                    Token la3 = obter_proximo_token(&copy);
                    Token la4 = obter_proximo_token(&copy);
                    if (la3.tipo == TOKEN_DELIMITADOR && strcmp(la3.lexema, "]") == 0 && la4.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(la4.lexema, "funcao") == 0) {
                        // parse parts
                        char tmp[64]; strncpy(tmp, la2.lexema, sizeof(tmp)-1); tmp[sizeof(tmp)-1] = '\0';
                        char *d = strchr(tmp, '.'); if (d) { *d = '\0'; declared_dec_before = atoi(tmp); declared_dec_after = atoi(d+1); }
                        is_func_decl = 1;
                    }
                } else {
                    Token la3 = obter_proximo_token(&copy);
                    Token la4 = obter_proximo_token(&copy);
                    Token la5 = obter_proximo_token(&copy);
                    if (la2.tipo == TOKEN_NUMERO_INT && la3.tipo == TOKEN_OPERADOR && strcmp(la3.lexema, ".") == 0 && la4.tipo == TOKEN_NUMERO_INT && la5.tipo == TOKEN_DELIMITADOR && strcmp(la5.lexema, "]") == 0) {
                        Token la6 = obter_proximo_token(&copy);
                        if (la6.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(la6.lexema, "funcao") == 0) {
                            declared_dec_before = atoi(la2.lexema);
                            declared_dec_after = atoi(la4.lexema);
                            is_func_decl = 1;
                        }
                    } else if (la2.tipo == TOKEN_NUMERO_INT && la3.tipo == TOKEN_DELIMITADOR && strcmp(la3.lexema, "]") == 0 && la4.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(la4.lexema, "funcao") == 0) {
                        declared_size = atoi(la2.lexema);
                        is_func_decl = 1;
                    }
                }
            } else if (la1.tipo == TOKEN_PALAVRA_RESERVADA && strcmp(la1.lexema, "funcao") == 0) {
                is_func_decl = 1;
            }
            if (is_func_decl) {
                // consume the type token from the real lexer
                next_token();
                // if '[' follows, consume size and the ']' now
                if (cur()->tipo == TOKEN_DELIMITADOR && strcmp(cur()->lexema, "[") == 0) {
                    next_token(); // consume '['
                    Token *numtok = cur();
                    // handle decimal A.B or single integer
                    if (numtok->tipo == TOKEN_NUMERO_DEC) {
                        char tmp[64]; strncpy(tmp, numtok->lexema, sizeof(tmp)-1); tmp[sizeof(tmp)-1] = '\0'; char *d = strchr(tmp, '.'); if (d) { *d='\0'; declared_dec_before = atoi(tmp); declared_dec_after = atoi(d+1); }
                        next_token();
                        if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho\n", cur()?cur()->linha:-1); exit(1); }
                        next_token();
                    } else {
                        if (!numtok || numtok->tipo != TOKEN_NUMERO_INT) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado numero inteiro dentro de '[' ']'\n", numtok?numtok->linha:-1); exit(1); }
                        declared_size = atoi(numtok->lexema);
                        if (declared_size <= 0) { fprintf(stderr, "ERRO SINTATICO: linha %d: tamanho invalido '%s' em colchetes\n", numtok->linha, numtok->lexema); exit(1); }
                        next_token();
                        if (!is_delim("]")) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado ']' apos tamanho\n", cur()?cur()->linha:-1); exit(1); }
                        next_token();
                    }
                }
                // now expect 'funcao'
                if (!(cur()->tipo == TOKEN_PALAVRA_RESERVADA && strcmp(cur()->lexema, "funcao") == 0)) { fprintf(stderr, "ERRO SINTATICO: linha %d: esperado 'funcao' apos tipo\n", cur()?cur()->linha:-1); exit(1); }
                next_token(); // consume 'funcao'
                parse_function(declared, declared_size, declared_dec_before, declared_dec_after);
                return;
            }
            // otherwise it's a declaration (do not consume type here; parse_declaration expects it at cur())
            parse_declaration(); return;
        }
    }
    if (t->tipo == TOKEN_IDENTIFICADOR_VAR) { parse_assignment(); return; }
    fprintf(stderr, "ERRO SINTATICO: linha %d: token inesperado '%s'\n", t->linha, t->lexema); exit(1);
}

void parse_program(void) {
    while (cur()->tipo != TOKEN_FIM_DE_ARQUIVO) { parse_statement(); }
    symtab_print();
    // ensure principal exists per specification
    if (!symtab_lookup_any("principal")) {
        fprintf(stderr, "Modulo Principal Inexistente\n");
        exit(1);
    }
}
