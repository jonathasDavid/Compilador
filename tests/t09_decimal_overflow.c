#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program =
        "decimal[2.2] !d;\n"
        "!d = 123.45;\n"
        "";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    return 0;
}
