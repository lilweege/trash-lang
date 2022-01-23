#include "compiler.h"
#include "stringview.h"
#include "io.h"
#include "tokenizer.h"
#include "parser.h"
#include "analyzer.h"
#include "generator.h"
#include "simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define COMPILE_MESSAGE(filename, msg, line, col, fmt, args) do { \
    fprintf(stderr, "%s:%zu:%zu: "msg": ", \
            filename, line, col); \
    vfprintf(stderr, fmt, args); \
    fprintf(stderr, "\n"); \
} while (0)

void _compileError(const char* filename, size_t line, size_t col, const char* fmt, va_list args) {
    COMPILE_MESSAGE(filename, "ERROR", line, col, fmt, args);
}

void compileErrorAt(const char* filename, size_t line, size_t col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _compileError(filename, line, col, fmt, args);
    va_end(args);
    exit(1);
}

void compileError(FileInfo info, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _compileError(info.filename, info.location.line, info.location.col, fmt, args);
    va_end(args);
    exit(1);
}

void printUsage() {
    printf(
        "Usage: trash [commands]\n"
        "Commands:\n"
        "    -r <file>    Simulate the program.\n"
        "    -c <file>    Compiles the file to NASM Linux x86-64 assembly.\n"
        "    -h           Displays this information.\n"
    );
}

void compileArguments(int argc, char** argv) {
    assert(argc > 0);
    if (argc == 1) {
        printUsage();
        exit(0);
    }

    char* arg1 = argv[1];
    size_t sz = strlen(arg1);
    char dash = arg1[0];
    if (sz <= 1 || dash != '-') {
        printUsage();
        exit(1);
    }

    char option = arg1[1];
    if (option == 'h') {
        printUsage();
        exit(0);
    }

    if (argc != 3) {
        printUsage();
        exit(1);
    }
    char* filename = argv[2];
    if (option == 'c') {
        compileFile(filename);
    }
    else if (option == 'r') {
        simulateFile(filename);
    }
    else {
        printUsage();
        exit(1);
    }
}

void compileFile(char* filename) {
    size_t fileSize;
    char* fileContent;
    readFileOrCrash(filename, &fileSize, &fileContent);
    StringView fileView = svNew(fileSize, fileContent);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    verifyProgram(filename, program);
    // TODO: get filename and remove extension
    generateProgram("a", program);
    free(fileContent);
}

void simulateFile(char* filename) {
    size_t fileSize;
    char* fileContent;
    readFileOrCrash(filename, &fileSize, &fileContent);
    StringView fileView = svNew(fileSize, fileContent);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    verifyProgram(filename, program);
    simulateProgram(program, STACK_SIZE);
    free(fileContent);
}

