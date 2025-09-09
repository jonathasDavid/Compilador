#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

// Token types exported by the lexer
typedef enum {
    TOKEN_IDENTIFICADOR_VAR,
    TOKEN_IDENTIFICADOR_FUNC,
    TOKEN_PALAVRA_RESERVADA,
    TOKEN_NUMERO_INT,
    TOKEN_NUMERO_DEC,
    TOKEN_OPERADOR,
    TOKEN_DELIMITADOR,
    TOKEN_TEXTO,
    TOKEN_COMENTARIO,
    TOKEN_ERRO_LEXICO,
    TOKEN_FIM_DE_ARQUIVO
} TokenType;

#define TAMANHO_MAX_LEXEMA 256
typedef struct {
    TokenType tipo;
    char lexema[TAMANHO_MAX_LEXEMA];
    int linha;
} Token;

typedef struct {
    char *codigo_fonte;
    int posicao_atual;
    int linha_atual;
    int owns_buffer;
} LexerState;

void inicializar_lexer(LexerState *estado, const char *codigo);
void lexer_init_from_file(LexerState *estado, const char *path);
Token obter_proximo_token(LexerState *estado);
void lexer_dispose(LexerState *estado);

#endif
