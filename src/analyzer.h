#ifndef ANALYZER_H
#define ANALYZER_H

#include "stringview.h"
#include "parser.h"

typedef enum {
    TYPE_NONE, // placeholder
    TYPE_U8, // uint8_t
    TYPE_I64, // int64_t
    TYPE_F64, // double
    // ...
    TYPE_COUNT
} Type;

// more generally, this should be a pointer to some actual data
// something large such as an array on the stack makes this not work
typedef union {
    uint8_t u8;
    int64_t i64;
    double f64;
} Value;


typedef struct {
    Type type;
    size_t arrSize; // 0 if scalar type
    Value* value;
} Expression;

typedef struct {
    StringView id;
    Type type;
    size_t arrSize; // 0 if scalar type
    Value* value; // simulation
    // mem addr
    // ...
} Symbol;

#include "hashmap.h"

const char* typeKindName(Type type);

void verifyProgram(AST* program);

void simulateProgram(AST* program);
void simulateStatements(AST* statement, HashMap* symbolTable);
void simulateStatement(AST* statement, HashMap* symbolTable);
void simulateConditional(AST* conditional, HashMap* symbolTable);
Expression evaluateExpression(AST* expression, HashMap* symbolTable);
Expression evaluateCall(AST* call, HashMap* symbolTable);

#endif // ANALYZER_H
