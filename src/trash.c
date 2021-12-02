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
    
    StringView fileView;
    int res = readFile(argv[1], &fileView);
    if (res != 0) {
        if (res == 1) {
            fprintf(stderr, "File error: %s\n", strerror(errno));
        }
        else if (res == 2) {
            fprintf(stderr, "Error: malloc failed\n");
        }
        return 1;
    }

    printf("%s\n", fileView.data);
    Tokenizer tokenizer = (Tokenizer) { .source = fileView };
    while (pollToken(&tokenizer)) {
        Token t = tokenizer.nextToken;
        printf("%s:\t\"%.*s\"\n", TokenKindNames[t.kind], (int)t.text.size, t.text.data);
        tokenizer.nextToken.kind = TOKEN_NONE;
    }

    free(fileView.data);

    return 0;
}

