#include <stdio.h>
#include "lexer.h"

int main(void) {
    const char *program = "texto[3] funcao __retorna_texto() { texto !s; !s = \"abcdef\"; retorno !s; }";
    LexerState ls; inicializar_lexer(&ls, program);
    Token t;
    do {
        t = obter_proximo_token(&ls);
        printf("tok tipo=%d lex='%s' linha=%d\n", t.tipo, t.lexema, t.linha);
    } while (t.tipo != TOKEN_FIM_DE_ARQUIVO);
    return 0;
}
