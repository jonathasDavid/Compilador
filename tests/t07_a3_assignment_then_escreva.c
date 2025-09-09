#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program = "texto[3] !a; !a = \"abcdef\"; escreva(!a);"; // assignment overflow then escreva should warn via A3
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("t07_a3_assignment_then_escreva: finished (ALERTA expected)\n");
    return 0;
}
