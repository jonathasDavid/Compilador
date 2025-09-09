#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program = "texto[3] !s2; !s2 = \"abcdef\";";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("t02_texto_overflow: finished (ALERTA expected)\n");
    return 0;
}
