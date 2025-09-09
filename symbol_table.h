#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

typedef enum { TYPE_INT, TYPE_TEXT, TYPE_DECIMAL, TYPE_UNKNOWN } VarType;

typedef struct Symbol {
    char name[64];
    VarType type;
    int line_decl;
    // additional metadata
    char scope[64]; // scope name: "global" or function name
    int is_function;
    VarType return_type;
    int size; // for texto[n] or decimal[n]
    int dec_before; // for decimal: digits before '.'
    int dec_after;  // for decimal: digits after '.'
    int last_value_size; // size of last known assigned value (0 = unknown)
    int last_dec_before; // last assigned decimal digits before '.' (0 = unknown)
    int last_dec_after;  // last assigned decimal digits after '.' (0 = unknown)
    struct Symbol *next;
} Symbol;

void symtab_init(void);
Symbol *symtab_lookup(const char *name);
Symbol *symtab_insert(const char *name, VarType type, int line, int size, int dec_before, int dec_after);
// scope helpers
void symtab_enter_scope(const char *scope_name);
void symtab_leave_scope(void);
Symbol *symtab_lookup_current(const char *name);
Symbol *symtab_lookup_any(const char *name);
void symtab_print(void);
void symtab_set_last_value_size(const char *name, int vsize);
int symtab_get_last_value_size(const char *name);
void symtab_set_last_decimal_parts(const char *name, int before, int after);
void symtab_get_last_decimal_parts(const char *name, int *before, int *after);

#endif
