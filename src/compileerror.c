#include "compileerror.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define COMPILE_MESSAGE(filename, msg, line, col, fmt, args) do { \
    fprintf(stderr, "%s:%zu:%zu: "msg": ", \
            filename, line, col); \
    vfprintf(stderr, fmt, args); \
    fprintf(stderr, "\n"); \
} while (0)

void _compileError(const char* filename, size_t line, size_t col, const char* fmt, va_list args) {
    COMPILE_MESSAGE(filename, "ERROR", line, col, fmt, args);
}

void compileErrorAt(const char* filename, size_t line, size_t col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _compileError(filename, line, col, fmt, args);
    va_end(args);
    exit(1);
}

void compileError(FileInfo info, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _compileError(info.filename, info.location.line, info.location.col, fmt, args);
    va_end(args);
    exit(1);
}
