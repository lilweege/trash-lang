#include "stringView.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// TODO: improve error handling
// 0 success
// 1 file error
// 2 malloc failed
int readFile(char* filename, StringView* outView) {
    assert(outView != NULL);
    // TODO: windows version of file manipulation
    
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

    // reserve buffer
    char* buff = (char*) malloc(size + 1);
    if (buff == NULL) {
        return 2;
    }
    
    // read content
    long numBytes = fread(buff, 1, size, fp);
    if (numBytes != size) {
        free(buff);
        return 1;
    }
    fclose(fp);
    buff[size] = 0;

    outView->data = buff;
    outView->size = size;
    return 0;
}
