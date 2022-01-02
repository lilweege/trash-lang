#include "compiler.h"
#include "stringView.h"
#include "io.h"
#include "tokenizer.h"
#include "parser.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void compileArguments(int argc, char** argv) {
    // TODO: add optional flags
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    compileFilename(argv[1]);
}

void compileFilename(char* filename) {
    StringView fileView;
    int res = readFile(filename, &fileView);
    if (res != 0) {
        if (res == 1) {
            fprintf(stderr, "File error: %s\n", strerror(errno));
        }
        else if (res == 2) {
            fprintf(stderr, "Error: malloc failed\n");
        }
        exit(1);
    }

    printf("Text: `%s`\n", fileView.data);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    (void) program;
    free(fileView.data);
}
