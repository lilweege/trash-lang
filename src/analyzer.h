#ifndef ANALYZER_H
#define ANALYZER_H

#include "stringview.h"

typedef enum {
    TYPE_STR, // StringView
    TYPE_I64, // int64_t
    TYPE_F64, // double
} Type;

typedef struct {
    StringView id;
    Type type;
    // more generally, this should be a pointer to some actual data
    // something large such as an array on the stack makes this not work
    union {
        StringView str;
        int64_t i64;
        double f64;
    }; // simulation
    // mem addr
    // ...
} Symbol;

#endif // ANALYZER_H
