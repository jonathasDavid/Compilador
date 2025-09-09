#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
	const char *program = "texto[3] !a; !a = \"abc\"; texto[4] !b; !b = \"abcdef\";";
	LexerState ls; inicializar_lexer(&ls, program);
	parser_init_from_lexer(&ls);
	parse_program();
	printf("t05_escreva_literal_overflow: finished (ALERTA expected)\n");
	return 0;
}
