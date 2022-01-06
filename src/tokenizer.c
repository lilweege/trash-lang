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

    size_t lines, cols;
    do {
        // whitespace
        lines = 0, cols = 0;
        svLeftTrim(&tokenizer->source, &lines, &cols);
        tokenizer->curLineNo += lines;
        tokenizer->curColNo += cols;

        // line comment beginning with '?'
        if (tokenizer->source.size == 0) {
            break;
        }
        char curChar = *tokenizer->source.data;
        if (curChar == '?') {
            size_t commentEnd = 0;
            if (!svFirstIndexOfChar(tokenizer->source, '\n', &commentEnd)) {
                break;
            }
            svLeftChop(&tokenizer->source, commentEnd);
            tokenizer->curLineNo++;
            tokenizer->curColNo = 0;
        }
    } while (lines != 0 || cols != 0);

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

        case '\'':
        case '"': {
            char delim = tokenizer->source.data[0];
            svLeftChop(&tokenizer->source, 1); // open quote
            size_t idx;
            for (idx = 0; idx < tokenizer->source.size; ++idx) {
                if (tokenizer->source.data[idx] == delim) {
                    break;
                }
                if (tokenizer->source.data[idx] == '\n') {
                    tokenizerFailAt(*tokenizer,
                            tokenizer->curLineNo,
                            tokenizer->curColNo+idx,
                            "Unexpected end of line in string literal");
                }
                if (tokenizer->source.data[idx] == '\\') {
                    ++idx;
                    if (idx >= tokenizer->source.size) {
                        tokenizerFailAt(*tokenizer,
                                tokenizer->curLineNo,
                                tokenizer->curColNo+idx,
                                "Unexpected end of file in string literal");
                    }
                    char escapeChar = tokenizer->source.data[idx];
                    if (escapeChar != delim && escapeChar != '\n' &&
                            escapeChar != 'n' && escapeChar != 't' && escapeChar != '\\') {
                        tokenizerFailAt(*tokenizer,
                                tokenizer->curLineNo,
                                tokenizer->curColNo+idx,
                                "Invalid escape sequence '\\%c'", escapeChar);
                    }
                }
            }
            if (idx >= tokenizer->source.size) {
                tokenizerFail(*tokenizer, "Unexpected end of file in string literal");
            }
            tokenizer->curColNo += 2; // quotes are not included in string literal token
            tokenizer->nextToken.kind = delim == '"' ? TOKEN_STRING_LITERAL : TOKEN_CHAR_LITERAL;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, idx);
            svLeftChop(&tokenizer->source, 1); // close quote
        } break;

        case '\\': assert(0 && "char \\ not implemented yet"); break;
        case '.': assert(0 && "char . not implemented yet"); break;
        // TODO: other operators and combinations

        default: {
            if (isalpha(curChar) || curChar == '_') {
                // identifier or keyword
                tokenizer->nextToken.kind = TOKEN_IDENTIFIER;
                tokenizer->nextToken.text = svLeftChopWhile(&tokenizer->source, isIdentifier);
                if (svCmp(svFromCStr("if"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_IF;
                }
                else if (svCmp(svFromCStr("else"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_ELSE;
                }
                else if (svCmp(svFromCStr("while"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_WHILE;
                }
                else if (svCmp(svFromCStr("u8"), tokenizer->nextToken.text) == 0 ||
                        svCmp(svFromCStr("i64"), tokenizer->nextToken.text) == 0 ||
                        svCmp(svFromCStr("f64"), tokenizer->nextToken.text) == 0) {
                    tokenizer->nextToken.kind = TOKEN_TYPE;
                }
            }
            else if (isdigit(curChar)) {
                // integer literal
                tokenizer->nextToken.kind = TOKEN_INTEGER_LITERAL;
                tokenizer->nextToken.text = svLeftChopWhile(&tokenizer->source, isNumeric);
                // float literal
                // for now, something like ".1" is not allowed
                // it must have a leading integer part, i.e. "0.1"
                if (svPeek(tokenizer->source, 0) == '.') {
                    tokenizer->nextToken.kind = TOKEN_FLOAT_LITERAL;
                    svLeftChop(&tokenizer->source, 1); // decimal point
                    StringView mantissa = svLeftChopWhile(&tokenizer->source, isNumeric);
                    tokenizer->nextToken.text.size += mantissa.size + 1;
                }
            }
            else {
                fprintf(stderr, "ERROR: Unhandled character '%c'\n", curChar);
                exit(1);
            }
        }
    }

    assert(tokenizer->nextToken.kind != TOKEN_NONE);
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

