#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv) {
    // TODO: add optional flags
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    char* fileContents;
    long fileSize;
    int res = readFile(argv[1], &fileContents, &fileSize);
    if (res != 0) {
        if (res == 1) {
            fprintf(stderr, "File error: %s\n", strerror(errno));
        }
        else if (res == 2) {
            fprintf(stderr, "Error: malloc failed\n");
        }
        return 1;
    }

    printf("%s\n", fileContents);
    free(fileContents);

    return 0;
}

