#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include "stringview.h"
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
    // and more ...
} TokenKind;

static const char* TokenKindNames[33] = {
    "TOKEN_NONE",
    "TOKEN_SEMICOLON",
    "TOKEN_IDENTIFIER",
    "TOKEN_IF",
    "TOKEN_ELSE",
    "TOKEN_WHILE",
    "TOKEN_TYPE",
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
    "TOKEN_FLOAT_LITERAL",
    "TOKEN_STRING_LITERAL",
    "TOKEN_CHAR_LITERAL",
};

typedef struct {
    TokenKind kind;
    StringView text;
    size_t lineNo, colNo;
} Token;

typedef struct {
    char* filename;
    StringView source;
    Token nextToken;
    size_t curLineNo, curColNo;
} Tokenizer;

void tokenizerFail(Tokenizer tokenizer, char* message, ...);
void tokenizerFailAt(Tokenizer tokenizer, size_t line, size_t col, char* message, ...);
bool pollToken(Tokenizer* tokenizer);

#endif // _TOKENIZER_H
