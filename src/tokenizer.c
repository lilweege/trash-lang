#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

// TODO: improve error handling
// 0 success
// 1 file error
// 2 malloc failed
int readFile(char* filename, StringView* outView) {
    assert(outView != NULL);
    // TODO: windows version of file manipulation
    
    // open file
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        return 1;
    }
    
    // get file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        return 1;
    }
    long size = ftell(fp);
    if (size== -1L) {
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

// consumed token == true
bool pollToken(Tokenizer* tokenizer) {
    if (tokenizer->nextToken.kind != TOKEN_NONE) {
        return false;
    }
    
    // trim leading whitespace
    // TODO: update tokenizer curLineNo
    while (tokenizer->source.size > 0 && isspace(*tokenizer->source.data)) {
        ++tokenizer->source.data;
        --tokenizer->source.size;
    }

    if (tokenizer->source.size == 0) {
        return false;
    }

    // TODO: refactor repetition
    char curChar = *tokenizer->source.data;
    switch (curChar) {
        case '(': {
            tokenizer->nextToken.kind = TOKEN_OPEN_PAREN;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case ')': {
            tokenizer->nextToken.kind = TOKEN_CLOSE_PAREN;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case '+': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_POS;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case '-': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_NEG;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case '*': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_MUL;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case '/': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_DIV;
            tokenizer->nextToken.text.data = tokenizer->source.data;
            tokenizer->nextToken.text.size = 1;
            tokenizer->source.data += 1;
            tokenizer->source.size -= 1;
        } break;

        case '%': assert(0 && "char % not implemented yet"); break;
        // handle = and == here
        case '=': assert(0 && "char = not implemented yet"); break;
        // TODO: bitwise operators
        case '"': assert(0 && "char \" not implemented yet"); break;
        case '\'': assert(0 && "char ' not implemented yet"); break;
        case '{': assert(0 && "char { not implemented yet"); break;
        case '}': assert(0 && "char } not implemented yet"); break;
        case ',': assert(0 && "char , not implemented yet"); break;
        case '.': assert(0 && "char . not implemented yet"); break;
        case '>': assert(0 && "char > not implemented yet"); break;
        case '<': assert(0 && "char < not implemented yet"); break;
        // TODO: other operators

        default: {
            if (curChar == '_' || isalpha(curChar)) {
                // identifier or keyword
                tokenizer->nextToken.kind = TOKEN_IDENTIFIER;
                tokenizer->nextToken.text.data = tokenizer->source.data;
                while (tokenizer->source.size > 0 &&
                        (isalpha(*tokenizer->source.data) || *tokenizer->source.data == '_')) {
                    ++tokenizer->nextToken.text.size;
                    ++tokenizer->source.data;
                    --tokenizer->source.size;
                }
                --tokenizer->nextToken.text.size;
            }
            else if (isdigit(curChar)) {
                // TODO: double literals
                // integer literal
                tokenizer->nextToken.kind = TOKEN_INTEGER_LITERAL;
                tokenizer->nextToken.text.data = tokenizer->source.data;
                while (tokenizer->source.size > 0 && isdigit(*tokenizer->source.data)) {
                    ++tokenizer->nextToken.text.size;
                    ++tokenizer->source.data;
                    --tokenizer->source.size;
                }
                --tokenizer->nextToken.text.size;
            }
            else {
                assert(0 && "Error: Unhandled character");
            }
        }
    }


    return true;
}

