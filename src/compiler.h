#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    size_t line, col;
} FileLocation;

typedef struct {
    const char* filename;
    FileLocation location;
} FileInfo;

void compileErrorAt(const char* filename, size_t line, size_t col, const char* fmt, ...);
void compileError(FileInfo info, const char* fmt, ...);

void compileFilename(char* filename);
void compileArguments(int argc, char** argv);

#endif // COMPILER_H
