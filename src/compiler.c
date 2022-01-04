#include "compiler.h"
#include "stringView.h"
#include "io.h"
#include "tokenizer.h"
#include "parser.h"

#include <stdio.h>
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
    StringView fileView = readFileOrCrash(filename);

    printf("Text: `%s`\n", fileView.data);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    (void) program;
    free(fileView.data);
}
