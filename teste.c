#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// --- Definição dos Tipos de Tokens ---
typedef enum {
    TOKEN_IDENTIFICADOR_VAR,   // Nomes de variáveis, ex: !minhaVariavel
    TOKEN_IDENTIFICADOR_FUNC,  // Nomes de funções, ex: __minhaFuncao
    TOKEN_PALAVRA_RESERVADA, // Palavras com significado especial, ex: se, inteiro
    TOKEN_NUMERO_INT,          // Números inteiros, ex: 10, 255
    TOKEN_NUMERO_DEC,          // Números de ponto flutuante, ex: 3.14
    TOKEN_OPERADOR,            // Símbolos de operação, ex: +, =, <>
    TOKEN_DELIMITADOR,         // Símbolos de estruturação, ex: (, }, ;
    TOKEN_TEXTO,               // Literais de texto, ex: "ola mundo"
    TOKEN_COMENTARIO,          // Comentários a serem ignorados
    TOKEN_ERRO_LEXICO,         // Símbolo ou sequência não reconhecida
    TOKEN_FIM_DE_ARQUIVO       // Marcador para o final do código fonte
} TokenType;

// --- Estrutura para o Token ---
#define TAMANHO_MAX_LEXEMA 100
typedef struct {
    TokenType tipo;
    char lexema[TAMANHO_MAX_LEXEMA];
    int linha;
} Token;

// --- Estado do Analisador Léxico ---
typedef struct {
    const char *codigo_fonte;
    int posicao_atual;
    int linha_atual;
} LexerState;

// Lista de palavras reservadas da sua linguagem (Case Sensitive)
const char *palavras_reservadas[] = {
    "principal", "funcao", "retorno", "leia", "escreva", "se", "senao", "para",
    "inteiro", "texto", "decimal", NULL
};

// --- Funções do Analisador ---

void inicializar_lexer(LexerState *estado, const char *codigo) {
    estado->codigo_fonte = codigo;
    estado->posicao_atual = 0;
    estado->linha_atual = 1;
}

int eh_palavra_reservada(const char *lexema) {
    for (int i = 0; palavras_reservadas[i] != NULL; i++) {
        if (strcmp(lexema, palavras_reservadas[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

Token obter_proximo_token(LexerState *estado) {
    Token token;
    char lexema_buffer[TAMANHO_MAX_LEXEMA] = {0};
    int lexema_idx = 0;
    char char_atual;

    while (isspace(estado->codigo_fonte[estado->posicao_atual])) {
        if (estado->codigo_fonte[estado->posicao_atual] == '\n') {
            estado->linha_atual++;
        }
        estado->posicao_atual++;
    }

    token.linha = estado->linha_atual;
    char_atual = estado->codigo_fonte[estado->posicao_atual];

    if (char_atual == '\0') {
        token.tipo = TOKEN_FIM_DE_ARQUIVO;
        strcpy(token.lexema, "EOF");
        return token;
    }

    // REGRA 2.4: Reconhecer identificador de variável (!a, !var1)
    if (char_atual == '!') {
        lexema_buffer[lexema_idx++] = char_atual;
        estado->posicao_atual++;
        // Após '!', deve vir uma letra minúscula
        if (islower(estado->codigo_fonte[estado->posicao_atual])) {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            // Após, pode ser qualquer letra ou número
            while (isalnum(estado->codigo_fonte[estado->posicao_atual])) {
                lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            }
            token.tipo = TOKEN_IDENTIFICADOR_VAR;
        } else {
            token.tipo = TOKEN_ERRO_LEXICO; // Formato de variável inválido
        }
        strcpy(token.lexema, lexema_buffer);
        return token;
    }

    // REGRA 1.4.1.1: Reconhecer identificador de função (__func, __func1)
    if (char_atual == '_') {
        if (estado->codigo_fonte[estado->posicao_atual + 1] == '_') {
            lexema_buffer[lexema_idx++] = '_';
            lexema_buffer[lexema_idx++] = '_';
            estado->posicao_atual += 2;
            // Após '__', pode ser letra ou número
            while (isalnum(estado->codigo_fonte[estado->posicao_atual])) {
                lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            }
            if (lexema_idx > 2) { // Precisa ter algo após o '__'
                 token.tipo = TOKEN_IDENTIFICADOR_FUNC;
            } else {
                 token.tipo = TOKEN_ERRO_LEXICO;
            }
            strcpy(token.lexema, lexema_buffer);
            return token;
        }
    }

    // Reconhecer Palavras Reservadas ou Erros (palavras soltas são erros)
    if (isalpha(char_atual)) {
        while (isalpha(estado->codigo_fonte[estado->posicao_atual])) {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        }
        token.tipo = eh_palavra_reservada(lexema_buffer) ? TOKEN_PALAVRA_RESERVADA : TOKEN_ERRO_LEXICO;
        strcpy(token.lexema, lexema_buffer);
        return token;
    }

    if (isdigit(char_atual)) {
        while (isdigit(estado->codigo_fonte[estado->posicao_atual])) {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        }
        if (estado->codigo_fonte[estado->posicao_atual] == '.') {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            while (isdigit(estado->codigo_fonte[estado->posicao_atual])) {
                lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
            }
            token.tipo = TOKEN_NUMERO_DEC;
        } else {
            token.tipo = TOKEN_NUMERO_INT;
        }
        strcpy(token.lexema, lexema_buffer);
        return token;
    }

    if (char_atual == '"') {
        estado->posicao_atual++;
        while (estado->codigo_fonte[estado->posicao_atual] != '"' && estado->codigo_fonte[estado->posicao_atual] != '\0') {
            lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
        }
        if (estado->codigo_fonte[estado->posicao_atual] == '"') {
            estado->posicao_atual++;
            token.tipo = TOKEN_TEXTO;
        } else {
            token.tipo = TOKEN_ERRO_LEXICO;
        }
        strcpy(token.lexema, lexema_buffer);
        return token;
    }

    switch (char_atual) {
        case '+': case '-': case '*': case '^': // Adicionado '^'
        case '(': case ')': case '{': case '}':
        case '[': case ']': // Adicionado '[]'
        case ';': case ',':
            token.tipo = (strchr("(){}[];,", char_atual)) ? TOKEN_DELIMITADOR : TOKEN_OPERADOR;
            lexema_buffer[0] = char_atual;
            strcpy(token.lexema, lexema_buffer);
            estado->posicao_atual++;
            return token;
        case '=':
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '=') {
                estado->posicao_atual++;
                strcpy(token.lexema, "==");
            } else {
                strcpy(token.lexema, "=");
            }
            token.tipo = TOKEN_OPERADOR;
            return token;
        case '<':
             estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '>') { // REGRA 3.2.2.2: Operador <>
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
        case '>':
             estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '=') {
                estado->posicao_atual++;
                strcpy(token.lexema, ">=");
            } else {
                strcpy(token.lexema, ">");
            }
            token.tipo = TOKEN_OPERADOR;
            return token;
        case '/':
            estado->posicao_atual++;
            if (estado->codigo_fonte[estado->posicao_atual] == '/') {
                estado->posicao_atual++;
                while(estado->codigo_fonte[estado->posicao_atual] != '\n' && estado->codigo_fonte[estado->posicao_atual] != '\0'){
                    lexema_buffer[lexema_idx++] = estado->codigo_fonte[estado->posicao_atual++];
                }
                token.tipo = TOKEN_COMENTARIO;
                strcpy(token.lexema, lexema_buffer);
                return token;
            } else {
                strcpy(token.lexema, "/");
                token.tipo = TOKEN_OPERADOR;
                return token;
            }
    }

    token.tipo = TOKEN_ERRO_LEXICO;
    lexema_buffer[0] = char_atual;
    strcpy(token.lexema, lexema_buffer);
    estado->posicao_atual++;
    return token;
}

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
