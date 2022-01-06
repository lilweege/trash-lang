#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "stringview.h"

typedef enum {
    TYPE_U8, // uint8_t
    TYPE_I64, // int64_t
    TYPE_F64, // double
} Type;

typedef struct {
    StringView id;
    Type type;
    // mem addr
    uint8_t* data; // simulation
    // ...
} Symbol;

#endif // _ANALYZER_H
