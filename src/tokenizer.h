#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include "stringView.h"
#include <stdbool.h>

typedef enum {
    TOKEN_NONE,
    TOKEN_NEWLINE,
    TOKEN_IDENTIFIER,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
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
    TOKEN_DOUBLE_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    // and more ...
} TokenKind;

static const char* TokenKindNames[32] = {
    "TOKEN_NONE",
    "TOKEN_NEWLINE",
    "TOKEN_IDENTIFIER",
    "TOKEN_IF",
    "TOKEN_ELSE",
    "TOKEN_WHILE",
    "TOKEN_OPERATOR_POS",
    "TOKEN_OPERATOR_NEG",
    "TOKEN_OPERATOR_MUL",
    "TOKEN_OPERATOR_DIV",
    "TOKEN_OPERATOR_MOD",
    "TOKEN_OPERATOR_COMMA",
    "TOKEN_OPERATOR_ASSIGN",
    "TOKEN_OPERATOR_EQ",
    "TOKEN_OPERATOR_NE",
    "TOKEN_OPERATOR_GE",
    "TOKEN_OPERATOR_GT",
    "TOKEN_OPERATOR_LE",
    "TOKEN_OPERATOR_LT",
    "TOKEN_OPERATOR_NOT",
    "TOKEN_OPERATOR_AND",
    "TOKEN_OPERATOR_OR",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
    "TOKEN_LCURLY",
    "TOKEN_RCURLY",
    "TOKEN_LSQUARE",
    "TOKEN_RSQUARE",
    "TOKEN_INTEGER_LITERAL",
    "TOKEN_DOUBLE_LITERAL",
    "TOKEN_STRING_LITERAL",
    "TOKEN_CHAR_LITERAL",
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

void tokenizerFail(Tokenizer tokenizer, char* message, ...);
bool pollToken(Tokenizer* tokenizer);

#endif // _TOKENIZER_H
