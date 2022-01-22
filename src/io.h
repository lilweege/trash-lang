#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define FILE_MALLOC_CAP (64 * 1024)
#define WRITER_MIN_SIZE (64 * 1024)

typedef struct {
    const char* fn;
    size_t cap;
    size_t size;
    char* data;
} FileWriter;

FileWriter fwCreate(const char* filename);
void fwDestroy(FileWriter fw);
bool fwGrow(FileWriter* fw);
void fwWriteChunkOrCrash(FileWriter* fw, char* fmt, ...);

void readFileOrCrash(const char* filename, size_t* outSize, char** outBuff);
void writeFileOrCrash(const char* filename, size_t size, char* buff);
int readFile(const char* filename, size_t* outSize, char** outBuff);
int writeFile(const char* filename, size_t size, char* buff);


#endif // IO_H
