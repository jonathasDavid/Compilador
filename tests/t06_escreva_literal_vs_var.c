#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program = "texto[3] !a; escreva(\"abcdef\", !a);"; // literal 6 > var size 3 -> ALERTA
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("t06_escreva_literal_vs_var: finished (ALERTA expected)\n");
    return 0;
}
