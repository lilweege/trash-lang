#include "tokenizer.h"
#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

const char* tokenKindName(TokenKind kind) {
    static_assert(TOKEN_COUNT == 33, "Exhaustive check of token kinds failed");
    const char* TokenKindNames[33] = {
        "TOKEN_NONE",
        "TOKEN_SEMICOLON",
        "TOKEN_IDENTIFIER",
        "TOKEN_IF",
        "TOKEN_ELSE",
        "TOKEN_WHILE",
        "TOKEN_TYPE",
        "TOKEN_OPERATOR_POS",
        "TOKEN_OPERATOR_NEG",
        "TOKEN_OPERATOR_MUL",
        "TOKEN_OPERATOR_DIV",
        "TOKEN_OPERATOR_MOD",
        "TOKEN_OPERATOR_COMMA",
        "TOKEN_OPERATOR_ASSIGN",
        "TOKEN_OPERATOR_EQ",
        "TOKEN_OPERATOR_NE",
        "TOKEN_OPERATOR_GE",
        "TOKEN_OPERATOR_GT",
        "TOKEN_OPERATOR_LE",
        "TOKEN_OPERATOR_LT",
        "TOKEN_OPERATOR_NOT",
        "TOKEN_OPERATOR_AND",
        "TOKEN_OPERATOR_OR",
        "TOKEN_LPAREN",
        "TOKEN_RPAREN",
        "TOKEN_LCURLY",
        "TOKEN_RCURLY",
        "TOKEN_LSQUARE",
        "TOKEN_RSQUARE",
        "TOKEN_INTEGER_LITERAL",
        "TOKEN_FLOAT_LITERAL",
        "TOKEN_STRING_LITERAL",
        "TOKEN_CHAR_LITERAL",
    };
    return TokenKindNames[kind];
}


// consumed token == true
bool pollToken(Tokenizer* tokenizer) {
    if (tokenizer->nextToken.kind != TOKEN_NONE) {
        return true;
    }

    size_t lineDiff, colDiff;
    do {
        // whitespace
        size_t linesBefore, colsBefore;
        linesBefore = tokenizer->curPos.line;
        colsBefore = tokenizer->curPos.col;
        svLeftTrim(&tokenizer->source, &tokenizer->curPos.line, &tokenizer->curPos.col);

        // line comment beginning with '?'
        if (tokenizer->source.size == 0) {
            break;
        }
        char curChar = *tokenizer->source.data;
        if (curChar == '?') {
            size_t commentEnd = 0;
            if (!svFirstIndexOfChar(tokenizer->source, '\n', &commentEnd)) {
                svLeftChop(&tokenizer->source, tokenizer->source.size);
                break;
            }
            svLeftChop(&tokenizer->source, commentEnd+1);
            tokenizer->curPos.line++;
            tokenizer->curPos.col = 0;
        }

        lineDiff = tokenizer->curPos.line - linesBefore;
        colDiff = tokenizer->curPos.col - colsBefore;
    } while (lineDiff != 0 || colDiff != 0);

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
                    compileErrorAt(tokenizer->filename,
                                   tokenizer->curPos.line + 1,
                                   tokenizer->curPos.col + idx + 1,
                                   "Unexpected end of line in string literal");
                }
                if (tokenizer->source.data[idx] == '\\') {
                    ++idx;
                    if (idx >= tokenizer->source.size) {
                        break;
                    }
                    char escapeChar = tokenizer->source.data[idx];
                    if (escapeChar != delim && escapeChar != '\n' &&
                            escapeChar != 'n' && escapeChar != 't' && escapeChar != '\\') {
                        compileErrorAt(tokenizer->filename,
                                       tokenizer->curPos.line + 1,
                                       tokenizer->curPos.col + idx + 1,
                                       "Invalid escape sequence '\\%c'", escapeChar);
                    }
                }
            }
            if (idx >= tokenizer->source.size) {
                compileErrorAt(tokenizer->filename,
                               tokenizer->curPos.line + 1,
                               tokenizer->curPos.col + idx + 1,
                               "Unexpected end of file in string literal");
            }
            tokenizer->curPos.col += 2; // quotes are not included in string literal token
            tokenizer->nextToken.kind = delim == '"' ? TOKEN_STRING_LITERAL : TOKEN_CHAR_LITERAL;
            tokenizer->nextToken.text = svLeftChop(&tokenizer->source, idx);
            if (tokenizer->nextToken.kind == TOKEN_CHAR_LITERAL) {
                if ((tokenizer->nextToken.text.size == 2 && tokenizer->nextToken.text.data[0] != '\\') ||
                        tokenizer->nextToken.text.size > 2) {
                    compileErrorAt(tokenizer->filename,
                                   tokenizer->nextToken.pos.line,
                                   tokenizer->nextToken.pos.col + 1,
                                   "Max length of character literal exceeded");
                }
            }
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
                        // svCmp(svFromCStr("u64"), tokenizer->nextToken.text) == 0 ||
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
                fprintf(stderr, "ERROR: Unhandled character %d: '%c'\n", curChar, curChar);
                exit(1);
            }
        }
    }

    assert(tokenizer->nextToken.kind != TOKEN_NONE);
    tokenizer->nextToken.pos.line = tokenizer->curPos.line + 1;
    tokenizer->nextToken.pos.col = tokenizer->curPos.col + 1;
    tokenizer->curPos.col += tokenizer->nextToken.text.size;

    printf("POLLED TOKEN <%s:%zu:%zu: \""SV_FMT"\">\n",
            tokenKindName(tokenizer->nextToken.kind),
            tokenizer->nextToken.pos.line+1,
            tokenizer->nextToken.pos.col+1,
            SV_ARG(tokenizer->nextToken.text));
    return true;
}

