#ifndef PARSER_H
#define PARSER_H

#include "symbol_table.h"
#include "lexer.h"

void parser_init_from_lexer(LexerState *ls);
void parse_program(void);

#endif
