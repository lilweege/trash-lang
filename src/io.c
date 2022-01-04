#include "io.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

StringView readFileOrCrash(char* filename) {
    StringView fileView;
    int res = readFile(filename, &fileView);
    if (res != 0) {
        if (res == -2) {
            fprintf(stderr, "ERROR: Malloc failed\n");
        }
        if (res == -1) {
            fprintf(stderr, "ERROR: File too large (%zu bytes)\n", fileView.size);
        }
        else if (res == 1) {
            fprintf(stderr, "ERROR: File error: %s\n", strerror(errno));
        }
        else if (res == 2) {
            fprintf(stderr, "ERROR: File error: Unexpected end of file\n");
        }
        else if (res == 3) {
            fprintf(stderr, "ERROR: File error: Could not read file\n");
        }
        else {
            fprintf(stderr, "ERROR: Uknown error: %s\n", strerror(errno));
        }
        exit(1);
    }

    return fileView;
}

// -2 malloc failed
// -1 malloc too big
// 0 success
// 1 file error (errno)
// 2 feof
// 3 ferror
int readFile(char* filename, StringView* outView) {
    assert(outView != NULL);
    
    // open file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        return 1;
    }
    
    // get file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        return 1;
    }
    long size = ftell(fp);
    if (size == -1L) {
        return 1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        return 1;
    }

    outView->size = size;
    if (size + 1 >= FILE_MALLOC_CAP) {
        return -1;
    }
    char* buff = (char*) malloc(size + 1);
    if (buff == NULL) {
        return -2;
    }
    
    // read content
    size_t numBytes = fread(buff, 1, size, fp);
    if (numBytes != (size_t)size) {
        free(buff);
        if (feof(fp)) {
            return 2;
        }
        else if (ferror(fp)) {
            return 3;
        }
        return 4; // this is probably unreachable
    }
    fclose(fp);
    buff[size] = 0;

    // caller is responsible for freeing this
    outView->data = buff;
    return 0;
}
