#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"

typedef enum {
    NODE_IF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_BLOCK,
    NODE_STATEMENT,
    NODE_CALL,
    NODE_DEFINITION,
    NODE_ARGUMENT,
    NODE_IDENTIFIER,
    NODE_TYPE,
    NODE_NUMBER,
    NODE_STRING,
    NODE_CHAR,
    NODE_ASSIGN,
    NODE_LVALUE,
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

static const char* NodeKindNames[30] = {
    "NODE_IF",
    "NODE_ELSE",
    "NODE_WHILE",
    "NODE_BLOCK",
    "NODE_STATEMENT",
    "NODE_CALL",
    "NODE_DEFINITION",
    "NODE_ARGUMENT",
    "NODE_IDENTIFIER",
    "NODE_TYPE",
    "NODE_NUMBER",
    "NODE_STRING",
    "NODE_CHAR",
    "NODE_ASSIGN",
    "NODE_LVALUE",
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
void printAST(AST* root, size_t depth);


// an incomplete (and informal) grammar definition...
// grammar symbols loosely include: { } [ ] |
// everything else is either a token kind, literal token text, or non-terminal
// (hopefully when I revisit this it's not completely unintelligible)
// TODO: bit arithmetic, pointers, arrays (strings will be u8 arrays), square bracket operator

// program         ->  { subroutine | statement }
// subroutine      ->  type identifier ( [ type identifier { , type identifier } ] ) block
// statement       ->  branch | definition | assignment | [ expression ] ;
// branch          ->  conditional ( expression )  block | statement
// block           ->  lcurly { statement } rcurly
// definition      ->  type identifier ;
// assignment      ->  lvalue = expression ;

// expression      ->  logicalTerm { || logicalTerm }
// logicalTerm     ->  logicalFactor { && logicalFactor }
// logicalFactor   ->  comparison { ==|!= comparison }
// comparison      ->  value { >=|<=|>|< value }
// value           ->  term { +|- term }
// term            ->  factor { *|/|% factor }
// factor          ->  integer | float | string | char | call | lvalue | ( expression ) | - factor | ! factor

// call            ->  identifier ( [ expression { , expression } ] )
// lvalue          ->  identifier [ subscript ]
// type            ->  primitive [ subscript ]
// subscript       ->  lsquare expression rsquare

// conditional     ->  "if" | "else" | "while"
// primitive       ->  "u8" | "i64" | "u64" | "f64" 


// other features (tbd)
// NOTE: I only choose these symbols because they are distinct from other symbols
// in the language, and therefore easier to lex (still subject to change)
// dereference     ->  #x
// address of      ->  @x
// comment         ->  ?...




AST* parseProgram(Tokenizer* tokenizer);
AST* parseSubroutine(Tokenizer* tokenizer);
AST* parseStatement(Tokenizer* tokenizer);
AST* parseBranch(Tokenizer* tokenizer);
AST* parseBlock(Tokenizer* tokenizer);
AST* parseDefinition(Tokenizer* tokenizer);
AST* parseAssignment(Tokenizer* tokenizer);
AST* parseExpression(Tokenizer* tokenizer);
AST* parseLogicalTerm(Tokenizer* tokenizer);
AST* parseLogicalFactor(Tokenizer* tokenizer);
AST* parseComparison(Tokenizer* tokenizer);
AST* parseValue(Tokenizer* tokenizer);
AST* parseTerm(Tokenizer* tokenizer);
AST* parseFactor(Tokenizer* tokenizer);
AST* parseCall(Tokenizer* tokenizer);
AST* parseLvalue(Tokenizer* tokenizer);
AST* parseType(Tokenizer* tokenizer);
AST* parseSubscript(Tokenizer* tokenizer);

#endif // PARSER_H
