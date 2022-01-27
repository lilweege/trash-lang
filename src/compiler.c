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
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

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

void execAndEcho(const char* command) {
    // command should be sanitized by now
    printf("%s\n", command);
    
    int result = system(command);
    if (result == 0) {
        return; // ok
    }

    if (result < 0) {
        fprintf(stderr, "ERROR: Could not execute command `%s`: %s\n",
                command, strerror(errno));
    }
    else {
        if (WIFEXITED(result)) {
            fprintf(stderr, "ERROR: Command `%s` returned %d\n",
                    command, WEXITSTATUS(result));
        }
        else {
            fprintf(stderr, "ERROR: Command `%s` exited abnormally\n",
                    command);
        }
    }
    exit(1);
}

void compileFile(char* filename) {
#define PATH_MAX 256
#define TMP_BUF_SZ 2 * PATH_MAX + 32
    static char cmdBuf[TMP_BUF_SZ];
    
    // read file
    size_t fileSize;
    char* fileContent;
    readFileOrCrash(filename, &fileSize, &fileContent);

    // check filename length
    size_t fnLen = strlen(filename);
    if (fnLen > PATH_MAX) {
        fprintf(stderr, "ERROR: File path length exceeded max path length of %d\n", PATH_MAX);
        exit(1);
    }
    StringView expectedExtension = svFromCStr(".trash");
    if (fnLen < expectedExtension.size + 1) {
        // not long enough to check file type
        fprintf(stderr, "ERROR: File did not have '"SV_FMT"' extension\n",
                SV_ARG(expectedExtension));
        exit(1);
    }

    // check extension
    const int pathnameLen = fnLen - expectedExtension.size;
    StringView fn = svFromCStr(filename);
    StringView actualExtension = svSubstring(fn, pathnameLen, fnLen);
    if (svCmp(expectedExtension, actualExtension) != 0) {
        fprintf(stderr, "ERROR: File did not have '"SV_FMT"' extension\n",
                SV_ARG(expectedExtension));
        exit(1);
    }

    StringView fileView = svNew(fileSize, fileContent);
    Tokenizer tokenizer = {
        .filename = filename,
        .source = fileView,
    };
    AST* program = parseProgram(&tokenizer);
    verifyProgram(filename, program);
    
    // get basename (output only in cwd)
    size_t pathEndIdx = 0;
    // TODO: windows compatibility with \\ path
    if (svLastIndexOfChar(fn, '/', &pathEndIdx)) {
        pathEndIdx += 1;
    }
    char outputFilename[PATH_MAX];
    memcpy(outputFilename, filename, pathnameLen);
    memcpy(outputFilename + pathnameLen, ".asm", 5);
    
    // generate the program
    char* asmFilename = outputFilename + pathEndIdx;
    generateProgram(asmFilename, program);
    printf("INFO: generated %s\n", asmFilename);
    free(fileContent);

    // assemble and link
    const int basenameLen = pathnameLen - pathEndIdx;
    snprintf(cmdBuf, TMP_BUF_SZ, "nasm -felf64"
#ifdef DEBUG
             " -g -wall"
#endif // DEBUG
             " -o %.*s.o %.*s.asm",
             basenameLen, asmFilename,
             basenameLen, asmFilename);
    printf("EXEC: ");
    execAndEcho(cmdBuf);

    snprintf(cmdBuf, TMP_BUF_SZ, "ld -o %.*s %.*s.o",
             basenameLen, asmFilename,
             basenameLen, asmFilename);
    printf("EXEC: ");
    execAndEcho(cmdBuf);

#undef PATH_MAX
#undef TMP_BUF_SZ
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

