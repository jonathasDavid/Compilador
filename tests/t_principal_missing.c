#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    const char *program = "inteiro !a; !a = 1;"; // no principal()
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("t_principal_missing: OK\n");
    return 0;
}
