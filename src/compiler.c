#include "compiler.h"
#include "stringview.h"
#include "io.h"
#include "tokenizer.h"
#include "parser.h"
#include "analyzer.h"

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
    size_t fileSize;
    char* fileContent;
    readFileOrCrash(filename, &fileSize, &fileContent);

    StringView fileView = svNew(fileSize, fileContent);

    printf("Text: `%s`\n", fileView.data);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    verifyProgram(program);
    simulateProgram(program);

    free(fileContent);
}
