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
    // TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_INTEGER_LITERAL,
    // and more ...
} TokenKind;


static const char* TokenKindNames[9] = {
    "TOKEN_NONE",
    "TOKEN_IDENTIFIER",
    "TOKEN_OPERATOR_POS",
    "TOKEN_OPERATOR_NEG",
    "TOKEN_OPERATOR_MUL",
    "TOKEN_OPERATOR_DIV",
    "TOKEN_OPEN_PAREN",
    "TOKEN_CLOSE_PAREN",
    "TOKEN_INTEGER_LITERAL",
};


typedef struct {
    TokenKind kind;
    StringView text;
} Token;


typedef struct {
    StringView source;
    Token nextToken;
    // int curLineNo;
} Tokenizer;


int readFile(char* filename, StringView* outView);
bool pollToken(Tokenizer* tokenizer);

#endif // _TOKENIZER_H
