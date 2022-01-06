#ifndef IO_H
#define IO_H

#include <stddef.h>

#define FILE_MALLOC_CAP (64 * 1024)

void readFileOrCrash(char* filename, size_t* outSize, char** outBuff);
int readFile(char *filename, size_t* outSize, char** outBuff);

#endif // IO_H
