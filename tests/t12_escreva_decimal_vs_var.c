#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program =
        "decimal[2.2] !d; !d = 12.34; escreva(123.456, !d);";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    return 0;
}
