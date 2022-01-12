#ifndef ANALYZER_H
#define ANALYZER_H

#include "stringview.h"
#include "parser.h"

typedef enum {
    TYPE_NONE, // placeholder
    TYPE_STR, // StringView
    TYPE_U8, // uint8_t
    TYPE_I64, // int64_t
    TYPE_F64, // double
    // ...
    TYPE_COUNT
} Type;

// more generally, this should be a pointer to some actual data
// something large such as an array on the stack makes this not work
typedef union {
    StringView sv;
    uint8_t u8;
    int64_t i64;
    double f64;
} Value;


typedef struct {
    Type type;
    int64_t index; // 0 if scalar, treat same
    Value* value;
} Expression;

typedef struct {
    StringView id;
    Type type;
    int64_t arrSize; // -1 if scalar type
    Value* value; // simulation
    // mem addr
    // ...
} Symbol;

#include "hashmap.h"

const char* typeKindName(Type type);

void verifyProgram(const char* filename, AST* program);

void simulateProgram(const char* filename, AST* program);
void simulateStatements(const char* filename, AST* statement, HashMap* symbolTable);
void simulateStatement(const char* filename, AST* statement, HashMap* symbolTable);
void simulateConditional(const char* filename, AST* conditional, HashMap* symbolTable);
Expression evaluateExpression(const char* filename, AST* expression, HashMap* symbolTable);
Expression evaluateCall(const char* filename, AST* call, HashMap* symbolTable);

#endif // ANALYZER_H
