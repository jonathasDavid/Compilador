#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "memmon.h"

const char *palavras_reservadas[] = {
    "principal", "funcao", "retorno", "leia", "escreva", "se", "senao", "para",
    "inteiro", "texto", "decimal", NULL
};

void inicializar_lexer(LexerState *estado, const char *codigo) {
    estado->codigo_fonte = (char*)codigo;
    estado->posicao_atual = 0;
    estado->linha_atual = 1;
    estado->owns_buffer = 0;
}

void lexer_init_from_file(LexerState *estado, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { estado->codigo_fonte = NULL; estado->posicao_atual = 0; estado->linha_atual = 1; estado->owns_buffer = 0; return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = (char*)mm_malloc(sz+1); if (!buf) { fclose(f); estado->codigo_fonte = NULL; return; }
    fread(buf,1,sz,f); buf[sz]=0; fclose(f);
    estado->codigo_fonte = buf; estado->posicao_atual = 0; estado->linha_atual = 1; estado->owns_buffer = 1;
}

int eh_palavra_reservada(const char *lexema) {
    for (int i = 0; palavras_reservadas[i] != NULL; i++) {
        if (strcmp(lexema, palavras_reservadas[i]) == 0) return 1;
    }
    return 0;
}

Token obter_proximo_token(LexerState *estado) {
    Token token; char lexema_buffer[TAMANHO_MAX_LEXEMA]; memset(lexema_buffer,0,sizeof(lexema_buffer)); int lexema_idx=0;
    while (estado->codigo_fonte && isspace((unsigned char)estado->codigo_fonte[estado->posicao_atual])) {
        if (estado->codigo_fonte[estado->posicao_atual] == '\n') estado->linha_atual++;
        estado->posicao_atual++;
    }
    token.linha = estado->linha_atual;
    if (!estado->codigo_fonte) { token.tipo = TOKEN_FIM_DE_ARQUIVO; strcpy(token.lexema, "EOF"); return token; }
    char char_atual = estado->codigo_fonte[estado->posicao_atual];
    if (char_atual == '\0') { token.tipo = TOKEN_FIM_DE_ARQUIVO; strcpy(token.lexema, "EOF"); return token; }
    if (char_atual == '!') {
        lexema_buffer[lexema_idx++] = char_atual; estado->posicao_atual++;
        if (islower((unsigned char)estado->codigo_fonte[estado->posicao_atual])) {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            while (isalnum((unsigned char)estado->codigo_fonte[estado->posicao_atual])) lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            token.tipo = TOKEN_IDENTIFICADOR_VAR;
        } else { token.tipo = TOKEN_ERRO_LEXICO; }
        strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
    }
    if (char_atual == '_') {
        if (estado->codigo_fonte[estado->posicao_atual+1] == '_') {
            lexema_buffer[lexema_idx++] = '_'; lexema_buffer[lexema_idx++] = '_'; estado->posicao_atual+=2;
            while (isalnum((unsigned char)estado->codigo_fonte[estado->posicao_atual])) lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            if (lexema_idx>2) token.tipo = TOKEN_IDENTIFICADOR_FUNC; else token.tipo = TOKEN_ERRO_LEXICO;
            strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
        }
    }
    if (isalpha((unsigned char)char_atual)) {
        while (isalpha((unsigned char)estado->codigo_fonte[estado->posicao_atual])) lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        token.tipo = eh_palavra_reservada(lexema_buffer) ? TOKEN_PALAVRA_RESERVADA : TOKEN_ERRO_LEXICO;
        strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
    }
    if (isdigit((unsigned char)char_atual)) {
        while (isdigit((unsigned char)estado->codigo_fonte[estado->posicao_atual])) lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        if (estado->codigo_fonte[estado->posicao_atual] == '.') {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            while (isdigit((unsigned char)estado->codigo_fonte[estado->posicao_atual])) lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            token.tipo = TOKEN_NUMERO_DEC;
        } else token.tipo = TOKEN_NUMERO_INT;
        strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
    }
    if (char_atual == '"') {
        estado->posicao_atual++;
        while (estado->codigo_fonte[estado->posicao_atual] != '"' && estado->codigo_fonte[estado->posicao_atual] != '\0') lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        if (estado->codigo_fonte[estado->posicao_atual] == '"') { estado->posicao_atual++; token.tipo = TOKEN_TEXTO; }
        else token.tipo = TOKEN_ERRO_LEXICO;
        strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
    }
    switch (char_atual) {
        case '+': case '-': case '*': case '^':
        case '(': case ')': case '{': case '}':
        case '[': case ']': case ';': case ',':
            token.tipo = (strchr("(){}[];,", char_atual)) ? TOKEN_DELIMITADOR : TOKEN_OPERADOR;
            lexema_buffer[0] = char_atual; lexema_buffer[1]=0; strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1);
            estado->posicao_atual++; return token;
        case '=': {
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '=') {
                estado->posicao_atual++;
                strcpy(token.lexema, "==");
            } else {
                strcpy(token.lexema, "=");
            }
            token.tipo = TOKEN_OPERADOR;
            return token;
        }
        case '<': {
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '>') {
                estado->posicao_atual++;
                strcpy(token.lexema, "<>");
            } else if (estado->codigo_fonte[estado->posicao_atual] == '=') {
                estado->posicao_atual++;
                strcpy(token.lexema, "<=");
            } else {
                strcpy(token.lexema, "<");
            }
            token.tipo = TOKEN_OPERADOR;
            return token;
        }
        case '>': {
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '=') {
                estado->posicao_atual++;
                strcpy(token.lexema, ">=");
            } else {
                strcpy(token.lexema, ">");
            }
            token.tipo = TOKEN_OPERADOR;
            return token;
        }
        case '/':
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '/') {
                estado->posicao_atual++;
                while (estado->codigo_fonte[estado->posicao_atual] != '\n' && estado->codigo_fonte[estado->posicao_atual] != '\0') lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
                token.tipo = TOKEN_COMENTARIO; strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); return token;
            } else { strcpy(token.lexema, "/"); token.tipo = TOKEN_OPERADOR; return token; }
    }
    token.tipo = TOKEN_ERRO_LEXICO; lexema_buffer[0] = char_atual; lexema_buffer[1]=0; strncpy(token.lexema, lexema_buffer, sizeof(token.lexema)-1); estado->posicao_atual++; return token;
}

void lexer_dispose(LexerState *estado) {
    if (estado->owns_buffer && estado->codigo_fonte) mm_free((void*)estado->codigo_fonte);
    estado->codigo_fonte = NULL;
}
