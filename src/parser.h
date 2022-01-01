#ifndef _PARSER_H
#define _PARSER_H

#include "tokenizer.h"

typedef enum {
    NODE_IF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_STATEMENT,
    NODE_BLOCK,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_ASSIGN,
    NODE_EQ,
    NODE_NE,
    NODE_GE,
    NODE_GT,
    NODE_LE,
    NODE_LT,
    NODE_NOT,
    NODE_AND,
    NODE_OR,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,
    NODE_NEG,
    // ...
} NodeKind;

static const char* NodeKindNames[23] = {
    "NODE_IF",
    "NODE_ELSE",
    "NODE_WHILE",
    "NODE_STATEMENT",
    "NODE_BLOCK",
    "NODE_IDENTIFIER",
    "NODE_NUMBER",
    "NODE_ASSIGN",
    "NODE_EQ",
    "NODE_NE",
    "NODE_GE",
    "NODE_GT",
    "NODE_LE",
    "NODE_LT",
    "NODE_NOT",
    "NODE_AND",
    "NODE_OR",
    "NODE_ADD",
    "NODE_SUB",
    "NODE_MUL",
    "NODE_DIV",
    "NODE_MOD",
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

// an incomplete pseudo grammar definition
// NOTE: the tokens themselves are not included here
// TODO: functions, conditional statements

// program         ->  { statement }
// statement       ->  branch | assignment | expression ;
// branch          ->  if | else | while  ( expression ) block
// block           ->  lcurly { statement } rcurly | statement
// assignment      ->  identifier = expression ;
// expression      ->  logicalTerm { || logicalTerm }
// logicalTerm     ->  logicalFactor { && logicalFactor }
// logicalFactor   ->  comparison { ==|!= comparison }
// comparison      ->  value { >=|<=|>|< value }
// value           ->  term { +|- term }
// term            ->  factor { *|/|% factor }
// factor          ->  number | identifier | ( expression ) | - factor | ! factor

AST* parseProgram(Tokenizer* tokenizer);
AST* parseStatement(Tokenizer* tokenizer);
AST* parseBranch(Tokenizer* tokenizer);
AST* parseBlock(Tokenizer* tokenizer);
AST* parseAssignment(Tokenizer* tokenizer);
AST* parseExpression(Tokenizer* tokenizer);
AST* parseLogicalTerm(Tokenizer* tokenizer);
AST* parseLogicalFactor(Tokenizer* tokenizer);
AST* parseComparison(Tokenizer* tokenizer);
AST* parseValue(Tokenizer* tokenizer);
AST* parseTerm(Tokenizer* tokenizer);
AST* parseFactor(Tokenizer* tokenizer);

#endif // _PARSER_H
