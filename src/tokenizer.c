#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

bool isIdentifier(char c) { return isalnum(c) || c == '_'; }
bool isNumeric(char c) { return isdigit(c); }

void _tokenizerError(Tokenizer tokenizer, size_t line, size_t col, char* fmt, va_list args) {
    fprintf(stderr, "%s:%zu:%zu: ERROR: ",
        tokenizer.filename,
        line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void tokenizerFailAt(Tokenizer tokenizer, size_t line, size_t col, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _tokenizerError(tokenizer, line+1, col+1, fmt, args);
    va_end(args);
    exit(1);
}

void tokenizerFail(Tokenizer tokenizer, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _tokenizerError(tokenizer, tokenizer.curLineNo+1, tokenizer.curColNo+1, fmt, args);
    va_end(args);
    exit(1);
}

// consumed token == true
bool pollToken(Tokenizer* tokenizer) {
    if (tokenizer->nextToken.kind != TOKEN_NONE) {
        return true;
    }

    // trim leading whitespace
    svLeftTrim(&tokenizer->source, &tokenizer->curLineNo, &tokenizer->curColNo);

    if (tokenizer->source.size == 0) {
        printf("POLLED NOTHING\n");
        return false;
    }

    char curChar = *tokenizer->source.data;
    switch (curChar) {
        case ';': {
            tokenizer->nextToken.kind = TOKEN_SEMICOLON;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '(': {
            tokenizer->nextToken.kind = TOKEN_LPAREN;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case ')': {
            tokenizer->nextToken.kind = TOKEN_RPAREN;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '{': {
            tokenizer->nextToken.kind = TOKEN_LCURLY;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '}': {
            tokenizer->nextToken.kind = TOKEN_RCURLY;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '[': {
            tokenizer->nextToken.kind = TOKEN_LSQUARE;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case ']': {
            tokenizer->nextToken.kind = TOKEN_RSQUARE;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '+': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_POS;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '-': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_NEG;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '*': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_MUL;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '/': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_DIV;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;
        
        case '%': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_MOD;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case ',': {
            tokenizer->nextToken.kind = TOKEN_OPERATOR_COMMA;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
        } break;

        case '=': {
            if (svPeek(tokenizer->source, 1) == '=') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_EQ;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_ASSIGN;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
            }
        } break;

        case '!': {
            if (svPeek(tokenizer->source, 1) == '=') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_NE;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_NOT;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
            }
        } break;

        
        // TODO: bitwise operators
        case '&': {
            if (svPeek(tokenizer->source, 1) == '&') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_AND;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                assert(0 && "&? not implemented yet");
            }
        } break;

        case '|': {
            if (svPeek(tokenizer->source, 1) == '|') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_OR;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                assert(0 && "|? not implemented yet");
            }
        } break;

        // TODO: bit shift
        case '>': {
            if (svPeek(tokenizer->source, 1) == '=') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_GE;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_GT;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
            }
        } break;

        case '<': {
            if (svPeek(tokenizer->source, 1) == '=') {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_LE;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 2);
            }
            else {
                tokenizer->nextToken.kind = TOKEN_OPERATOR_LT;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, 1);
            }
        } break;

        case '"': {
            // NOTE: currently this is a raw string
            // TODO: single line string
            // TODO: escape sequences
            svLeftChop(&tokenizer->source, 1); // open quote
            int64_t idx = svFirstIndexOfChar(tokenizer->source, '"');
            if (idx == -1) {
                tokenizerFail(*tokenizer, "Unmatched \"");
            }
            else {
                tokenizer->nextToken.kind = TOKEN_STRING_LITERAL;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, idx);
                svLeftChop(&tokenizer->source, 1); // close quote
            }
        } break;

        case '\'': {
            svLeftChop(&tokenizer->source, 1); // open quote
            int64_t idx = svFirstIndexOfChar(tokenizer->source, '\'');
            if (idx == -1) {
                tokenizerFail(*tokenizer, "Unmatched '");
            }
            else {
                tokenizer->nextToken.kind = TOKEN_CHAR_LITERAL;
                tokenizer->nextToken.text = svLeftChop(&tokenizer->source, idx);
                svLeftChop(&tokenizer->source, 1); // close quote
            }
        } break;

        // TODO: bitwise operators
        case '\\': assert(0 && "char \\ not implemented yet"); break;
        case '.': assert(0 && "char . not implemented yet"); break;
        // TODO: other operators and combinations

        default: {
            if (isalpha(curChar) || curChar == '_') {
                // identifier or keyword
                tokenizer->nextToken.kind = TOKEN_IDENTIFIER;
                tokenizer->nextToken.text = svLeftChopWhile(&tokenizer->source, isIdentifier);
                // TODO: keywords
                if (svCmp(svFromCStr("if"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_IF;
                }
                else if (svCmp(svFromCStr("else"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_ELSE;
                }
                else if (svCmp(svFromCStr("while"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_WHILE;
                }
                // else if (svCmp(svFromCStr("func"), tokenizer->nextToken.text) == 0) {
                //     // ...
                // }
            }
            else if (isdigit(curChar)) {
                // integer literal
                tokenizer->nextToken.kind = TOKEN_INTEGER_LITERAL;
                tokenizer->nextToken.text = svLeftChopWhile(&tokenizer->source, isNumeric);
                // double literal
                // for now, something like ".1" is not allowed
                // it must have a leading integer part, i.e. "0.1"
                if (svPeek(tokenizer->source, 0) == '.') {
                    tokenizer->nextToken.kind = TOKEN_DOUBLE_LITERAL;
                    svLeftChop(&tokenizer->source, 1); // decimal point
                    StringView mantissa = svLeftChopWhile(&tokenizer->source, isNumeric);
                    tokenizer->nextToken.text.size += mantissa.size + 1;
                }
            }
            else {
                assert(0 && "Error: Unhandled character");
            }
        }
    }

    // NOTE: doesn't work with current raw string literals
    tokenizer->nextToken.lineNo = tokenizer->curLineNo;
    tokenizer->nextToken.colNo = tokenizer->curColNo;
    tokenizer->curColNo += tokenizer->nextToken.text.size;

    printf("POLLED TOKEN <%s:%zu:%zu: \""SV_FMT"\">\n",
            TokenKindNames[tokenizer->nextToken.kind],
            tokenizer->nextToken.lineNo+1,
            tokenizer->nextToken.colNo+1,
            SV_ARG(tokenizer->nextToken.text));
    return true;
}

