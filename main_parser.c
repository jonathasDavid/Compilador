#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "memmon.h"

int main(void) {
    const char *program =
        "inteiro !a, !b2 = 7;\n"
        "escreva(\"Escreva um numero\", !b2);\n"
        "!a = !b2 + 3;\n"
        "se(!a <= !b2) escreva(\"A<=B\", !a);\n"
    "funcao __soma(inteiro !e, inteiro !aa) { inteiro !num; !num = !e + !aa; retorno !num; }\n"
        "";

    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    memmon_report_peak();
    return 0;
}
