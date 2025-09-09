#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

static int run_case(const char *name, const char *program) {
    printf("== Test: %s\n", name);
    LexerState ls; inicializar_lexer(&ls, program);
    parser_init_from_lexer(&ls);
    parse_program();
    printf("-- Test '%s' finished (no fatal error)\n\n", name);
    return 0;
}

int main(void) {
    // 1) happy path: texto with size, assignment of shorter literal
    const char *t1 = "texto[10] !s; !s = \"abc\";";
    run_case("texto-happy", t1);

    // 2) overflow: texto[3] assigned a longer literal -> should emit ALERTA but continue
    const char *t2 = "texto[3] !s2; !s2 = \"abcdef\";";
    run_case("texto-overflow", t2);

    // 3) syntax error: missing ']' in declaration
    const char *t3 = "texto[5 !s3;"; // malformed
    run_case("texto-missing-bracket", t3);

    return 0;
}
