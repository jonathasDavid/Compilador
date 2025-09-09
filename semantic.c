#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* We'll keep a single file-scoped current function name variable for return checks. */
static char semantic_current_function[64] = "";

void semantic_init(void) {
    // nothing for now
}

void semantic_check_declaration(const char *name, VarType type, int line, int size, int dec_before, int dec_after) {
    // insert into symbol table
    if (!symtab_insert(name, type, line, size, dec_before, dec_after)) {
        fprintf(stderr, "ERRO: linha %d: simbolo duplicado ou memoria insuficiente ao inserir '%s'\\n", line, name);
        exit(1);
    }
}

void semantic_check_assignment(const char *name, VarType rtype, int line, int rsize, int rdec_after) {
    Symbol *s = symtab_lookup_any(name);
    if (!s) {
        fprintf(stderr, "ERRO: linha %d: Variavel '%s' nao declarada\\n", line, name);
        exit(1);
    }
    if (s->type != rtype && rtype != TYPE_UNKNOWN) {
        fprintf(stderr, "ALERTA SEMANTICO: linha %d: atribuicao de tipo diferente para '%s' (decl: %d, rhs: %d)\\n", line, name, s->type, rtype);
        // continue execution
    }
    // check sizes for texto/decimal
    if (s->type == TYPE_TEXT && s->size > 0 && rsize > 0) {
        if (rsize > s->size) {
            fprintf(stderr, "ALERTA SEMANTICO: linha %d: atribuicao excede tamanho do destino '%s' (dest: %d, rhs: %d)\\n", line, name, s->size, rsize);
        }
    } else if (s->type == TYPE_DECIMAL) {
        // for decimal, compare digits before/after
        if (s->dec_before > 0 && rsize >= 0) {
            if (rsize > s->dec_before) {
                fprintf(stderr, "ALERTA SEMANTICO: linha %d: atribuicao excede parte inteira do decimal '%s' (decl before: %d, rhs before: %d)\\n", line, name, s->dec_before, rsize);
            }
        }
        if (s->dec_after > 0 && rdec_after >= 0) {
            if (rdec_after > s->dec_after) {
                fprintf(stderr, "ALERTA SEMANTICO: linha %d: atribuicao excede parte fracionaria do decimal '%s' (decl after: %d, rhs after: %d)\\n", line, name, s->dec_after, rdec_after);
            }
        }
    }
}

void semantic_declare_function(const char *fname, VarType ret_type, int line, int size) {
    // insert function symbol as a special symbol
    Symbol *s = symtab_insert(fname, TYPE_UNKNOWN, line, size, 0, 0);
    if (!s) { fprintf(stderr, "ERRO: linha %d: nao foi possivel declarar funcao '%s'\\n", line, fname); exit(1); }
    s->is_function = 1;
    s->return_type = ret_type;
}

void semantic_enter_function(const char *fname) {
    symtab_enter_scope(fname);
    strncpy(semantic_current_function, fname, sizeof(semantic_current_function)-1);
    semantic_current_function[sizeof(semantic_current_function)-1] = '\0';
}

void semantic_leave_function(void) {
    semantic_current_function[0] = '\0';
    symtab_leave_scope();
}

void semantic_check_return(VarType rtype, int rsize, int rdec_after, int line) {
    if (semantic_current_function[0] == '\0') {
        fprintf(stderr, "ERRO: linha %d: 'retorno' fora de funcao\n", line);
        exit(1);
    }
    Symbol *f = symtab_lookup_any(semantic_current_function);
    if (!f) {
        fprintf(stderr, "ERRO: linha %d: simbolo de funcao '%s' nao encontrado\n", line, semantic_current_function);
        exit(1);
    }
    int declared_size = f->size; // for texto
    int declared_before = f->dec_before;
    int declared_after = f->dec_after;
    if (f->return_type == TYPE_UNKNOWN) {
        /* first return: set function return type */
        f->return_type = rtype;
        // if function had no declared size, record the returned size as observed
        if (rtype == TYPE_TEXT) {
            if (declared_size == 0) f->size = rsize;
        } else if (rtype == TYPE_DECIMAL) {
            if (declared_before == 0 && declared_after == 0) {
                f->dec_before = rsize;
                f->dec_after = rdec_after;
            }
        }
    } else {
        if (f->return_type != rtype && rtype != TYPE_UNKNOWN) {
            fprintf(stderr, "ALERTA SEMANTICO: linha %d: retorno de tipo diferente na funcao '%s' (declarado: %d, retorno: %d)\n", line, semantic_current_function, f->return_type, rtype);
        }
    }
    // check sizes against the declared size (if any)
    if (f->return_type == TYPE_TEXT && declared_size > 0 && rsize > 0) {
        if (rsize > declared_size) {
            fprintf(stderr, "ALERTA SEMANTICO: linha %d: retorno excede tamanho declarado da funcao '%s' (decl: %d, retorno: %d)\\n", line, semantic_current_function, declared_size, rsize);
        }
    } else if (f->return_type == TYPE_DECIMAL) {
        if (declared_before > 0 && rsize > 0 && rsize > declared_before) {
            fprintf(stderr, "ALERTA SEMANTICO: linha %d: retorno excede parte inteira declarada da funcao '%s' (decl before: %d, retorno before: %d)\\n", line, semantic_current_function, declared_before, rsize);
        }
        if (declared_after > 0 && rdec_after > 0 && rdec_after > declared_after) {
            fprintf(stderr, "ALERTA SEMANTICO: linha %d: retorno excede parte fracionaria declarada da funcao '%s' (decl after: %d, retorno after: %d)\\n", line, semantic_current_function, declared_after, rdec_after);
        }
    }
}
