#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    // TODO: add optional flags
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    compileFilename(argv[1]);
    return 0;
}
