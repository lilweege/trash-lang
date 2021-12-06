#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include "stringView.h"
#include <stdbool.h>

typedef enum {
    TOKEN_NONE,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR_POS,
    TOKEN_OPERATOR_NEG,
    TOKEN_OPERATOR_MUL,
    TOKEN_OPERATOR_DIV,
    TOKEN_OPERATOR_MOD,
    TOKEN_OPERATOR_COMMA,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_EQ,
    TOKEN_OPERATOR_GE,
    TOKEN_OPERATOR_GT,
    TOKEN_OPERATOR_LE,
    TOKEN_OPERATOR_LT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_INTEGER_LITERAL,
    TOKEN_STRING_LITERAL,
    // and more ...
} TokenKind;

static const char* TokenKindNames[20] = {
    "TOKEN_NONE",
    "TOKEN_IDENTIFIER",
    "TOKEN_OPERATOR_POS",
    "TOKEN_OPERATOR_NEG",
    "TOKEN_OPERATOR_MUL",
    "TOKEN_OPERATOR_DIV",
    "TOKEN_OPERATOR_MOD",
    "TOKEN_OPERATOR_COMMA",
    "TOKEN_OPERATOR_ASSIGN",
    "TOKEN_OPERATOR_EQ",
    "TOKEN_OPERATOR_GE",
    "TOKEN_OPERATOR_GT",
    "TOKEN_OPERATOR_LE",
    "TOKEN_OPERATOR_LT",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_LBRACE",
    "TOKEN_RBRACE",
    "TOKEN_INTEGER_LITERAL",
    "TOKEN_STRING_LITERAL",
};

typedef struct {
    TokenKind kind;
    StringView text;
} Token;

typedef struct {
    char* filename;
    StringView source;
    Token nextToken;
    size_t curLineNo;
} Tokenizer;

bool pollToken(Tokenizer* tokenizer);
void tokenizerFail(Tokenizer tokenizer, char* message);

#endif // _TOKENIZER_H
