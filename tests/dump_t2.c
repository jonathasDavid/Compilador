#include <stdio.h>
#include "lexer.h"

int main(void){
    const char *program = "texto[3] !s2; !s2 = \"abcdef\";";
    LexerState ls; inicializar_lexer(&ls, program);
    while (1) {
        Token t = obter_proximo_token(&ls);
        if (t.tipo == TOKEN_FIM_DE_ARQUIVO) { printf("EOF\n"); break; }
        printf("tok type=%d lex='%s' line=%d\n", (int)t.tipo, t.lexema, t.linha);
    }
    return 0;
}
