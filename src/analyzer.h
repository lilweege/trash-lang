#ifndef ANALYZER_H
#define ANALYZER_H

#include "stringview.h"
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
    StringView id;
    Type type;
    // TODO: mem addr for simulation
} Symbol;

#include "hashmap.h"

const char* typeKindName(TypeKind type);
const char* typeKindKeyword(TypeKind type);

void verifyProgram(const char* filename, AST* program);
void verifyStatements(const char* filename, AST* statement, HashMap* symbolTable);
void verifyStatement(const char* filename, AST* statement, HashMap* symbolTable);
void verifyConditional(const char* filename, AST* conditional, HashMap* symbolTable);
Type checkCall(const char* filename, AST* call, HashMap* symbolTable);
Type checkExpression(const char* filename, AST* expression, HashMap* symbolTable);

#endif // ANALYZER_H
