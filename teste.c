#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "lexer.h"
#include <string.h>

// (single definitions above are used)

// --- ESTRUTURA E EXECUÇÃO DOS TESTES ---

const char* tipo_para_string(TokenType tipo) {
    switch (tipo) {
        case TOKEN_IDENTIFICADOR_VAR:   return "IDENTIFICADOR_VAR";
        case TOKEN_IDENTIFICADOR_FUNC:  return "IDENTIFICADOR_FUNC";
        case TOKEN_PALAVRA_RESERVADA:   return "PALAVRA_RESERVADA";
        case TOKEN_NUMERO_INT:          return "NUMERO_INT";
        case TOKEN_NUMERO_DEC:          return "NUMERO_DEC";
        case TOKEN_OPERADOR:            return "OPERADOR";
        case TOKEN_DELIMITADOR:         return "DELIMITADOR";
        case TOKEN_TEXTO:               return "TEXTO";
        case TOKEN_COMENTARIO:          return "COMENTARIO";
        case TOKEN_ERRO_LEXICO:         return "ERRO_LEXICO";
        case TOKEN_FIM_DE_ARQUIVO:      return "FIM_DE_ARQUIVO";
        default:                        return "DESCONHECIDO";
    }
}

void executar_teste(const char* nome_teste, const char* codigo_entrada, TokenType* tokens_esperados) {
    printf("--- Testando: %s ---\n", nome_teste);
    LexerState estado;
    inicializar_lexer(&estado, codigo_entrada);
    int i = 0;
    Token token;
    int falhou = 0;
    do {
        token = obter_proximo_token(&estado);
        if (token.tipo != tokens_esperados[i]) {
            printf("FALHOU! No token %d:\n", i + 1);
            printf("  - Esperado: %s\n", tipo_para_string(tokens_esperados[i]));
            printf("  - Obtido:   %s (Lexema: '%s')\n", tipo_para_string(token.tipo), token.lexema);
            falhou = 1;
            break;
        }
        if (token.tipo != TOKEN_COMENTARIO && token.tipo != TOKEN_FIM_DE_ARQUIVO) {
             printf("  Token %02d: %-20s Lexema: '%s'\n", i+1, tipo_para_string(token.tipo), token.lexema);
        }
        i++;
    } while (token.tipo != TOKEN_FIM_DE_ARQUIVO);

    if (!falhou) printf("PASSOU!\n\n"); else printf("\n");
}

int main() {
    printf("====================================================\n");
    printf("   SUITE DE TESTES DO LEXER - REGRAS ESPECIFICAS    \n");
    printf("====================================================\n\n");

    // Teste 1: Declaração de variáveis com a nova sintaxe
    const char* teste1_codigo = "inteiro !valor1 = 100;";
    TokenType teste1_esperado[] = {
        TOKEN_PALAVRA_RESERVADA, TOKEN_IDENTIFICADOR_VAR, TOKEN_OPERADOR, TOKEN_NUMERO_INT, TOKEN_DELIMITADOR,
        TOKEN_FIM_DE_ARQUIVO
    };
    executar_teste("Sintaxe de Variavel (!var)", teste1_codigo, teste1_esperado);

    // Teste 2: Estrutura 'se' com o novo operador relacional <>
    const char* teste2_codigo = "se (!a <> !b) { }";
    TokenType teste2_esperado[] = {
        TOKEN_PALAVRA_RESERVADA, TOKEN_DELIMITADOR, TOKEN_IDENTIFICADOR_VAR, TOKEN_OPERADOR, TOKEN_IDENTIFICADOR_VAR, TOKEN_DELIMITADOR, TOKEN_DELIMITADOR, TOKEN_DELIMITADOR,
        TOKEN_FIM_DE_ARQUIVO
    };
    executar_teste("Operador Relacional <>", teste2_codigo, teste2_esperado);

    // Teste 3: Declaração de função e exponenciação
    const char* teste3_codigo = "funcao __calc() { !c = !a ^ 2; }";
    TokenType teste3_esperado[] = {
        TOKEN_PALAVRA_RESERVADA, TOKEN_IDENTIFICADOR_FUNC, TOKEN_DELIMITADOR, TOKEN_DELIMITADOR, TOKEN_DELIMITADOR,
        TOKEN_IDENTIFICADOR_VAR, TOKEN_OPERADOR, TOKEN_IDENTIFICADOR_VAR, TOKEN_OPERADOR, TOKEN_NUMERO_INT, TOKEN_DELIMITADOR,
        TOKEN_DELIMITADOR,
        TOKEN_FIM_DE_ARQUIVO
    };
    executar_teste("Sintaxe de Funcao (__) e Operador ^", teste3_codigo, teste3_esperado);

    // Teste 4: Erro Léxico (variável com formato incorreto)
    const char* teste4_codigo = "inteiro !1valor;";
    TokenType teste4_esperado[] = {
        TOKEN_PALAVRA_RESERVADA, TOKEN_ERRO_LEXICO, TOKEN_FIM_DE_ARQUIVO
    };
    executar_teste("Erro - Variavel com formato invalido", teste4_codigo, teste4_esperado);

    // Teste 5: Erro Léxico (palavra solta que não é reservada)
    const char* teste5_codigo = "minhaVariavel = 10;";
     TokenType teste5_esperado[] = {
        TOKEN_ERRO_LEXICO, TOKEN_OPERADOR, TOKEN_NUMERO_INT, TOKEN_DELIMITADOR, TOKEN_FIM_DE_ARQUIVO
    };
    executar_teste("Erro - Palavra solta nao reservada", teste5_codigo, teste5_esperado);

    return 0;
}
