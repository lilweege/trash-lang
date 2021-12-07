#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

typedef enum {
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_NEG,
    // ...
} NodeKind;

static const char* NodeKindNames[7] = {
    "NODE_IDENTIFIER",
    "NODE_NUMBER",
    "NODE_ADD",
    "NODE_SUB",
    "NODE_MUL",
    "NODE_DIV",
    "NODE_NEG",
};


typedef struct AST AST;
struct AST {
    NodeKind kind;
    Token token;
    AST *left, *right;
};
// both nodes being NULL means terminal node (value)
// right can be NULL, which represents a unary node
// otherwise, the node is a binary node

AST* newNode(NodeKind kind, Token token);

void parseTokens(Tokenizer tokenizer);
void printAST(AST* root, size_t depth);
AST* parseExpression(Tokenizer* tokenizer);
AST* parseTerm(Tokenizer* tokenizer);
AST* parseFactor(Tokenizer* tokenizer);

#endif // _PARSER_H
