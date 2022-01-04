#ifndef _IO_H
#define _IO_H

#include "stringview.h"

#define FILE_MALLOC_CAP (64 * 1024)

StringView readFileOrCrash(char* filename);
int readFile(char *filename, StringView *outView);

#endif // _IO_H
