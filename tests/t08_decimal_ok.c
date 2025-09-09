#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program =
        "decimal[2.3] !d;\n"
        "!d = 12.345;\n"
        "";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    return 0;
}
