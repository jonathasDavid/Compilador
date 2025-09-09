#include <stdio.h>
#include <stdlib.h>
#include "symbol_table.h"

int main(void) {
    symtab_init();
    // insert a decimal variable
    Symbol *s = symtab_insert("!x", TYPE_DECIMAL, 1, 0, 0, 0);
    if (!s) { fprintf(stderr, "FAIL: symtab_insert returned NULL\n"); return 1; }

    // initially last parts should be zero
    int b = -1, a = -1;
    symtab_get_last_decimal_parts("!x", &b, &a);
    if (b != 0 || a != 0) { fprintf(stderr, "FAIL: expected initial 0.0, got %d.%d\n", b, a); return 1; }

    // set last decimal parts and read them back
    symtab_set_last_decimal_parts("!x", 3, 4);
    b = a = -1;
    symtab_get_last_decimal_parts("!x", &b, &a);
    if (b == 3 && a == 4) { fprintf(stdout, "PASS: symtab_get_last_decimal_parts returned 3.4\n"); return 0; }
    fprintf(stderr, "FAIL: expected 3.4, got %d.%d\n", b, a);
    return 1;
}
