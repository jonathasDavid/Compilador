#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include "memmon.h"
#include <string.h>

static Symbol *symbols = NULL;
static char current_scope[64] = "global";

static int name_in_scope(const char *name, const char *scope) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0 && strcmp(s->scope, scope) == 0) return 1;
        s = s->next;
    }
    return 0;
}

void symtab_init(void) {
    symbols = NULL;
}

Symbol *symtab_lookup(const char *name) {
    return symtab_lookup_any(name);
}

Symbol *symtab_insert(const char *name, VarType type, int line, int size, int dec_before, int dec_after) {
    if (name_in_scope(name, current_scope)) return NULL; // duplicate in same scope
    Symbol *s = (Symbol*)mm_malloc(sizeof(Symbol));
    if (!s) return NULL;
    strncpy(s->name, name, sizeof(s->name)-1);
    s->name[sizeof(s->name)-1] = '\0';
    s->type = type;
    s->line_decl = line;
    strncpy(s->scope, current_scope, sizeof(s->scope)-1);
    s->scope[sizeof(s->scope)-1] = '\0';
    s->is_function = 0;
    s->return_type = TYPE_UNKNOWN;
    s->size = size;
    s->dec_before = dec_before;
    s->dec_after = dec_after;
    s->last_value_size = 0;
    s->last_dec_before = 0;
    s->last_dec_after = 0;
    s->next = symbols;
    symbols = s;
    return s;
}

void symtab_set_last_value_size(const char *name, int vsize) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) { s->last_value_size = vsize; return; }
        s = s->next;
    }
}

void symtab_set_last_decimal_parts(const char *name, int before, int after) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) {
            s->last_dec_before = before;
            s->last_dec_after = after;
            return;
        }
        s = s->next;
    }
}

void symtab_get_last_decimal_parts(const char *name, int *before, int *after) {
    *before = 0; *after = 0;
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) { *before = s->last_dec_before; *after = s->last_dec_after; return; }
        s = s->next;
    }
}

int symtab_get_last_value_size(const char *name) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) return s->last_value_size;
        s = s->next;
    }
    return 0;
}

void symtab_enter_scope(const char *scope_name) {
    strncpy(current_scope, scope_name, sizeof(current_scope)-1);
    current_scope[sizeof(current_scope)-1] = '\0';
}

void symtab_leave_scope(void) {
    strncpy(current_scope, "global", sizeof(current_scope)-1);
    current_scope[sizeof(current_scope)-1] = '\0';
}

Symbol *symtab_lookup_current(const char *name) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0 && strcmp(s->scope, current_scope) == 0) return s;
        s = s->next;
    }
    return NULL;
}

Symbol *symtab_lookup_any(const char *name) {
    Symbol *s = symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) return s;
        s = s->next;
    }
    return NULL;
}

void symtab_print(void) {
    printf("\nTabela de Simbolos:\n");
    Symbol *s = symbols;
    while (s) {
        const char *t = "UNKNOWN";
        if (s->type == TYPE_INT) t = "inteiro";
        else if (s->type == TYPE_TEXT) t = "texto";
        else if (s->type == TYPE_DECIMAL) t = "decimal";
        if (s->is_function) {
            const char *rt = "UNKNOWN";
            if (s->return_type == TYPE_INT) rt = "inteiro";
            else if (s->return_type == TYPE_TEXT) rt = "texto";
            else if (s->return_type == TYPE_DECIMAL) rt = "decimal";
            if (s->return_type == TYPE_TEXT || s->return_type == TYPE_DECIMAL) {
                printf("  %s - funcao retorna %s[%d] (decl linha %d)\n", s->name, rt, s->size, s->line_decl);
            } else {
                printf("  %s - funcao retorna %s (decl linha %d)\n", s->name, rt, s->line_decl);
            }
        } else if (s->type == TYPE_TEXT) {
            if (s->last_value_size > 0 && s->last_value_size != s->size && s->size > 0) {
                printf("  %s - %s[%d] (decl linha %d) [ultimo: %d]\n", s->name, t, s->size, s->line_decl, s->last_value_size);
            } else if (s->last_value_size > 0 && s->size == 0) {
                printf("  %s - %s[%d] (decl linha %d) [ultimo: %d]\n", s->name, t, s->size, s->line_decl, s->last_value_size);
            } else {
                printf("  %s - %s[%d] (decl linha %d)\n", s->name, t, s->size, s->line_decl);
            }
        } else if (s->type == TYPE_DECIMAL) {
            if ((s->last_dec_before > 0 || s->last_dec_after > 0) && (s->last_dec_before != s->dec_before || s->last_dec_after != s->dec_after)) {
                printf("  %s - %s[%d.%d] (decl linha %d) [ultimo: %d.%d]\n", s->name, t, s->dec_before, s->dec_after, s->line_decl, s->last_dec_before, s->last_dec_after);
            } else {
                printf("  %s - %s[%d.%d] (decl linha %d)\n", s->name, t, s->dec_before, s->dec_after, s->line_decl);
            }
        } else {
            printf("  %s - %s (decl linha %d)\n", s->name, t, s->line_decl);
        }
        s = s->next;
    }
}
