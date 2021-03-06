#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "stringview.h"
#include "compiler.h"
#include <stdbool.h>

typedef enum {
    TOKEN_NONE,
    TOKEN_SEMICOLON,
    TOKEN_IDENTIFIER,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_TYPE,
    TOKEN_OPERATOR_POS,
    TOKEN_OPERATOR_NEG,
    TOKEN_OPERATOR_MUL,
    TOKEN_OPERATOR_DIV,
    TOKEN_OPERATOR_MOD,
    TOKEN_OPERATOR_COMMA,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_EQ,
    TOKEN_OPERATOR_NE,
    TOKEN_OPERATOR_GE,
    TOKEN_OPERATOR_GT,
    TOKEN_OPERATOR_LE,
    TOKEN_OPERATOR_LT,
    TOKEN_OPERATOR_NOT,
    TOKEN_OPERATOR_AND,
    TOKEN_OPERATOR_OR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_LSQUARE,
    TOKEN_RSQUARE,
    TOKEN_INTEGER_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    // ...
    TOKEN_COUNT
} TokenKind;
const char* tokenKindName(TokenKind kind);


typedef struct {
    TokenKind kind;
    StringView text;
    FileLocation pos;
} Token;

typedef struct {
    char* filename;
    StringView source;
    Token nextToken;
    FileLocation curPos;
} Tokenizer;

bool pollToken(Tokenizer* tokenizer);

#endif // TOKENIZER_H
