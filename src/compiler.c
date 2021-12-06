#include "stringView.h"
#include "io.h"
#include "tokenizer.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

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
    while (pollToken(&tokenizer)) {
        Token t = tokenizer.nextToken;
        printf("%s:\t\"%.*s\"\n", TokenKindNames[t.kind], (int)t.text.size, t.text.data);
        tokenizer.nextToken.kind = TOKEN_NONE;
    }

    free(fileView.data);
}
