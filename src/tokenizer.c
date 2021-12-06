#include "tokenizer.h"

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

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

