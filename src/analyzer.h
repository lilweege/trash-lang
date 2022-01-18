#ifndef ANALYZER_H
#define ANALYZER_H

#include "parser.h"

typedef enum {
    TYPE_NONE, // void
    TYPE_STR, // char*
    TYPE_U8, // uint8_t
    TYPE_I64, // int64_t
    TYPE_F64, // double
    // ...
    TYPE_COUNT
} TypeKind;

typedef struct {
    TypeKind kind;
    size_t size; // 0 for scalar
} Type;

typedef struct {
    Type type;
    size_t offset; // from stack base
} Value;

typedef struct {
    StringView id;
    Value val;
} Symbol;

#include "hashmap.h"

// arbitrary limit
#define MAX_FUNC_ARGS 16

const char* typeKindName(TypeKind type);
const char* typeKindKeyword(TypeKind type);
TypeKind unaryResultTypeKind(TypeKind type, NodeKind op);
TypeKind binaryResultTypeKind(TypeKind type1, TypeKind type2, NodeKind op);

void verifyProgram(const char* filename, AST* program);
void verifyStatements(const char* filename, AST* statement, HashMap* symbolTable);
void verifyConditional(const char* filename, AST* statement, HashMap* symbolTable);
void verifyStatement(const char* filename, AST* wrapper, HashMap* symbolTable);
Type checkCall(const char* filename, AST* call, HashMap* symbolTable);
Type checkExpression(const char* filename, AST* expression, HashMap* symbolTable);

#endif // ANALYZER_H
