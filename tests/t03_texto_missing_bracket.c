#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program = "texto[5 !s3;"; // malformed
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("t03_texto_missing_bracket: should not reach here\n");
    return 0;
}
