#ifndef COMPILE_ERROR_H
#define COMPILE_ERROR_H

#include <stddef.h>
#include "fileinfo.h"

void compileErrorAt(const char* filename, size_t line, size_t col, const char* fmt, ...);
void compileError(FileInfo info, const char* fmt, ...);

#endif // COMPILE_ERROR_H
