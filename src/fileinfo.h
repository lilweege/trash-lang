#ifndef FILEINFO_H
#define FILEINFO_H

#include <stddef.h>

typedef struct {
    size_t line, col;
} FileLocation;

typedef struct {
    const char* filename;
    FileLocation location;
} FileInfo;

#endif // FILEINFO_H
