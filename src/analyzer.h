#ifndef ANALYZER_H
#define ANALYZER_H

#include "stringview.h"
#include "parser.h"

typedef enum {
    TYPE_U8, // uint8_t
    TYPE_I64, // int64_t
    TYPE_U64, // uint64_t
    TYPE_F64, // double
} Type;

// more generally, this should be a pointer to some actual data
// something large such as an array on the stack makes this not work
typedef union {
    uint8_t u8;
    int64_t i64;
    uint64_t u64;
    double f64;
} value;

typedef struct {
    StringView id;
    Type type;
    size_t arrSize; // 0 if scalar type
    value* value; // simulation
    // mem addr
    // ...
} Symbol;

#include "hashmap.h"

void verifyProgram(AST* program);


#endif // ANALYZER_H
