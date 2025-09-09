#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program =
        "decimal[2.2] !a; decimal !b; !b = 123.45; !a = !b;";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    return 0;
}
