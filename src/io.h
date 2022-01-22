#ifndef IO_H
#define IO_H

#include "stringview.h"
#include <stddef.h>
#include <stdio.h>

#define FILE_MALLOC_CAP (64 * 1024)
#define WRITER_MIN_SIZE (64 * 1024)

typedef struct {
    char* fn;
    size_t cap;
    size_t size;
    char* data;
} FileWriter;

FileWriter fwCreate(char* filename);
void fwDestroy(FileWriter fw);
bool fwGrow(FileWriter* fw);
void fwWriteOrCrash(FileWriter* fw, size_t size, char* buff);

void readFileOrCrash(char* filename, size_t* outSize, char** outBuff);
void writeFileOrCrash(char* filename, size_t size, char* buff);
int readFile(char* filename, size_t* outSize, char** outBuff);
int writeFile(char* filename, size_t size, char* buff);


#endif // IO_H
