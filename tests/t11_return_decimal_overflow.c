#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program =
        "decimal[2.2] funcao __f() { retorno 123.45; }";
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    return 0;
}
