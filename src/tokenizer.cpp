#include "tokenizer.hpp"
#include "compileerror.hpp"

#include <cassert>
#include <algorithm>
#include <array>
#include <iostream>

const char* TokenKindName(TokenKind kind) {
    static_assert(static_cast<uint32_t>(TokenKind::TOKEN_COUNT) == 43, "Exhaustive check of token kinds failed");
    const std::array<const char*, static_cast<uint32_t>(TokenKind::TOKEN_COUNT)> TokenKindNames{
        "TokenKind::NONE",
        "TokenKind::COMMENT",
        "TokenKind::SEMICOLON",
        "TokenKind::IDENTIFIER",
        "TokenKind::IF",
        "TokenKind::ELSE",
        "TokenKind::FOR",
        "TokenKind::PROC",
        "TokenKind::LET",
        "TokenKind::MUT",
        "TokenKind::RETURN",
        "TokenKind::BREAK",
        "TokenKind::CONTINUE",
        "TokenKind::U8",
        "TokenKind::I64",
        "TokenKind::F64",
        "TokenKind::OPERATOR_POS",
        "TokenKind::OPERATOR_NEG",
        "TokenKind::OPERATOR_MUL",
        "TokenKind::OPERATOR_DIV",
        "TokenKind::OPERATOR_MOD",
        "TokenKind::OPERATOR_COMMA",
        "TokenKind::OPERATOR_ASSIGN",
        "TokenKind::OPERATOR_EQ",
        "TokenKind::OPERATOR_NE",
        "TokenKind::OPERATOR_GE",
        "TokenKind::OPERATOR_GT",
        "TokenKind::OPERATOR_LE",
        "TokenKind::OPERATOR_LT",
        "TokenKind::OPERATOR_NOT",
        "TokenKind::OPERATOR_AND",
        "TokenKind::OPERATOR_OR",
        "TokenKind::LPAREN",
        "TokenKind::RPAREN",
        "TokenKind::LCURLY",
        "TokenKind::RCURLY",
        "TokenKind::LSQUARE",
        "TokenKind::RSQUARE",
        "TokenKind::ARROW",
        "TokenKind::INTEGER_LITERAL",
        "TokenKind::FLOAT_LITERAL",
        "TokenKind::STRING_LITERAL",
        "TokenKind::CHAR_LITERAL",
    };
    return TokenKindNames[static_cast<uint32_t>(kind)];
}

static void LeftTrim(Tokenizer& tokenizer, uint32_t* outLineNo, uint32_t* outColNo) {
    for (;
        tokenizer.sourceIdx < tokenizer.source.size();
        ++tokenizer.sourceIdx)
    {
        char ch = tokenizer.source[tokenizer.sourceIdx];
        if (!(ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t')) {
            break;
        }
        if (outLineNo != nullptr) {
            if (ch == '\n') {
                ++*outLineNo;
            }
        }
        if (outColNo != nullptr) {
            if (ch == '\n') {
                *outColNo = 0;
            }
            else {
                ++*outColNo;
            }
        }
    }
}


static std::string_view LeftChop(Tokenizer& tokenizer, size_t n) {
    std::string_view view{&tokenizer.source[tokenizer.sourceIdx], n};
    tokenizer.sourceIdx += n;
    return view;
}

static std::string_view LeftChopWhile(Tokenizer& tokenizer, auto predicate) {
    const auto* begin = tokenizer.source.cbegin() + static_cast<long>(tokenizer.sourceIdx);
    const auto* end = std::find_if(begin, tokenizer.source.cend(), [&predicate](char c) { return !predicate(c); } );
    tokenizer.sourceIdx += static_cast<size_t> (end - begin);
    return std::string_view{begin, end};
}

static TokenizerResult PollTokenWithComments(Tokenizer& tokenizer) {
    if (tokenizer.curToken.kind != TokenKind::NONE) {
        return { .err = TokenizerError::NONE };
    }

    size_t lineDiff = 0, colDiff = 0;
    do {
        // whitespace
        size_t linesBefore = tokenizer.curPos.line;
        size_t colsBefore = tokenizer.curPos.col;
        LeftTrim(tokenizer, &tokenizer.curPos.line, &tokenizer.curPos.col);

        // line comment beginning with '?'
        if (tokenizer.sourceIdx >= tokenizer.source.size()) {
            break;
        }
        char curChar = tokenizer.source[tokenizer.sourceIdx];
        if (curChar == '?') {
            // it is a comment
            tokenizer.curToken.kind = TokenKind::COMMENT;
            size_t commentEnd = tokenizer.source.find_first_of('\n', tokenizer.sourceIdx);
            if (commentEnd == std::string::npos) {
                // no newline implies end of file
                tokenizer.curToken.text = std::string_view{&tokenizer.source[tokenizer.sourceIdx]};
                tokenizer.sourceIdx = tokenizer.source.size();

                tokenizer.curToken.pos.line = tokenizer.curPos.line + 1;
                tokenizer.curToken.pos.col = tokenizer.curPos.col + 1;
                tokenizer.curPos.col += tokenizer.curToken.text.size();
                return { .err = TokenizerError::NONE };
            }
            tokenizer.curToken.text = LeftChop(tokenizer, commentEnd-tokenizer.sourceIdx);
            tokenizer.curToken.pos.line = tokenizer.curPos.line + 1;
            tokenizer.curToken.pos.col = tokenizer.curPos.col + 1;
            tokenizer.curPos.col += tokenizer.curToken.text.size();
            return { .err = TokenizerError::NONE };
        }

        lineDiff = tokenizer.curPos.line - linesBefore;
        colDiff = tokenizer.curPos.col - colsBefore;
    } while (lineDiff != 0 || colDiff != 0);

    if (tokenizer.sourceIdx >= tokenizer.source.size()) {
        return { .err = TokenizerError::EMPTY };
    }

    auto TokenUnimplemnted = [&tokenizer](std::string_view token) -> TokenizerResult {
        return {
            .err = TokenizerError::FAIL,
            .msg = CompileErrorMessage(
                tokenizer.filename,
                tokenizer.curPos.line + 1,
                tokenizer.curPos.col + 1,
                '"', token, "\" is not implemented yet"),
        };
    };

    char curChar = tokenizer.source[tokenizer.sourceIdx];
    switch (curChar) {
        case ';': {
            tokenizer.curToken.kind = TokenKind::SEMICOLON;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '(': {
            tokenizer.curToken.kind = TokenKind::LPAREN;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case ')': {
            tokenizer.curToken.kind = TokenKind::RPAREN;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '{': {
            tokenizer.curToken.kind = TokenKind::LCURLY;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '}': {
            tokenizer.curToken.kind = TokenKind::RCURLY;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '[': {
            tokenizer.curToken.kind = TokenKind::LSQUARE;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case ']': {
            tokenizer.curToken.kind = TokenKind::RSQUARE;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '+': {
            tokenizer.curToken.kind = TokenKind::OPERATOR_POS;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '-': {
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '>')
            {
                tokenizer.curToken.kind = TokenKind::ARROW;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                tokenizer.curToken.kind = TokenKind::OPERATOR_NEG;
                tokenizer.curToken.text = LeftChop(tokenizer, 1);
            }
        } break;

        case '*': {
            tokenizer.curToken.kind = TokenKind::OPERATOR_MUL;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '/': {
            tokenizer.curToken.kind = TokenKind::OPERATOR_DIV;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '%': {
            tokenizer.curToken.kind = TokenKind::OPERATOR_MOD;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case ',': {
            tokenizer.curToken.kind = TokenKind::OPERATOR_COMMA;
            tokenizer.curToken.text = LeftChop(tokenizer, 1);
        } break;

        case '=': {
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '=')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_EQ;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                tokenizer.curToken.kind = TokenKind::OPERATOR_ASSIGN;
                tokenizer.curToken.text = LeftChop(tokenizer, 1);
            }
        } break;

        case '!': {
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '=')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_NE;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                tokenizer.curToken.kind = TokenKind::OPERATOR_NOT;
                tokenizer.curToken.text = LeftChop(tokenizer, 1);
            }
        } break;

        case '&': {
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '&')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_AND;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                // TODO: bitwise and
                return TokenUnimplemnted("&");
            }
        } break;

        case '|': {
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '|')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_OR;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                // TODO: bitwise or
                return TokenUnimplemnted("|");
            }
        } break;

        case '>': {
            // TODO: right shift
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() ||
                tokenizer.source[tokenizer.sourceIdx + 1] == '=')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_GE;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                tokenizer.curToken.kind = TokenKind::OPERATOR_GT;
                tokenizer.curToken.text = LeftChop(tokenizer, 1);
            }
        } break;

        case '<': {
            // TODO: left shift
            if (tokenizer.sourceIdx + 1 < tokenizer.source.size() &&
                tokenizer.source[tokenizer.sourceIdx + 1] == '=')
            {
                tokenizer.curToken.kind = TokenKind::OPERATOR_LE;
                tokenizer.curToken.text = LeftChop(tokenizer, 2);
            }
            else {
                tokenizer.curToken.kind = TokenKind::OPERATOR_LT;
                tokenizer.curToken.text = LeftChop(tokenizer, 1);
            }
        } break;

        case '\'':
        case '"': {
            char delim = LeftChop(tokenizer, 1)[0];
            size_t idx = 0;
            for (idx = tokenizer.sourceIdx;
                idx < tokenizer.source.size();
                ++idx)
            {
                if (tokenizer.source[idx] == delim) {
                    break;
                }
                if (tokenizer.source[idx] == '\n') {
                    return {
                        .err = TokenizerError::FAIL,
                        .msg = CompileErrorMessage(
                            tokenizer.filename,
                            tokenizer.curPos.line + 1,
                            tokenizer.curPos.col + 1,
                            "Unexpected end of line in string literal"),
                    };
                }
                if (tokenizer.source[idx] == '\\') {
                    if (++idx >= tokenizer.source.size()) {
                        break;
                    }
                    char escapeChar = tokenizer.source[idx];
                    if (escapeChar != delim && escapeChar != '\n' &&
                        escapeChar != 'n' && escapeChar != 't' && escapeChar != '\\')
                    {
                        return {
                            .err = TokenizerError::FAIL,
                            .msg = CompileErrorMessage(
                                tokenizer.filename,
                                tokenizer.curPos.line + 1,
                                tokenizer.curPos.col + idx - tokenizer.sourceIdx + 1,
                                "Invalid escape sequence"),
                        };
                    }
                }
            }
            FileLocation strStart = { .line = tokenizer.curPos.line + 1, .col = tokenizer.curPos.col + 1, };
            if (idx >= tokenizer.source.size()) {
                return {
                    .err = TokenizerError::FAIL,
                    .msg = CompileErrorMessage(
                        tokenizer.filename, strStart.line, strStart.col,
                        "Unexpected end of file in string literal"),
                };
            }
            tokenizer.curPos.col += 2; // quotes are not included in string literal token
            tokenizer.curToken.kind = delim == '"' ? TokenKind::STRING_LITERAL : TokenKind::CHAR_LITERAL;
            tokenizer.curToken.text = LeftChop(tokenizer, idx-tokenizer.sourceIdx);
            if (tokenizer.curToken.kind == TokenKind::CHAR_LITERAL) {
                if ((tokenizer.curToken.text.size() == 2 && tokenizer.curToken.text[0] != '\\') ||
                    tokenizer.curToken.text.size() > 2)
                {
                    return {
                        .err = TokenizerError::FAIL,
                        .msg = CompileErrorMessage(
                            tokenizer.filename, strStart.line, strStart.col,
                            "Max length of character literal exceeded"),
                    };
                }
            }
            ++tokenizer.sourceIdx;
        } break;

        case '\\': return TokenUnimplemnted("\\");
        case '.': return TokenUnimplemnted(".");
        // TODO: other operators and combinations

        default: {
            auto IsIdentifier = [](char c) { return (isalnum(c) != 0) || c == '_'; };
            auto IsNumeric = [](char c) { return isdigit(c) != 0; };

            if ((isalpha(curChar) != 0) || curChar == '_') {
                // identifier or keyword
                tokenizer.curToken.kind = TokenKind::IDENTIFIER;
                tokenizer.curToken.text = LeftChopWhile(tokenizer, IsIdentifier);
                if (tokenizer.curToken.text == "if")            tokenizer.curToken.kind = TokenKind::IF;
                else if (tokenizer.curToken.text == "else")     tokenizer.curToken.kind = TokenKind::ELSE;
                else if (tokenizer.curToken.text == "for")      tokenizer.curToken.kind = TokenKind::FOR;
                else if (tokenizer.curToken.text == "u8")       tokenizer.curToken.kind = TokenKind::U8;
                else if (tokenizer.curToken.text == "i64")      tokenizer.curToken.kind = TokenKind::I64;
                else if (tokenizer.curToken.text == "f64")      tokenizer.curToken.kind = TokenKind::F64;
                else if (tokenizer.curToken.text == "proc")     tokenizer.curToken.kind = TokenKind::PROC;
                else if (tokenizer.curToken.text == "let")      tokenizer.curToken.kind = TokenKind::LET;
                else if (tokenizer.curToken.text == "mut")      tokenizer.curToken.kind = TokenKind::MUT;
                else if (tokenizer.curToken.text == "return")   tokenizer.curToken.kind = TokenKind::RETURN;
                else if (tokenizer.curToken.text == "break")    tokenizer.curToken.kind = TokenKind::BREAK;
                else if (tokenizer.curToken.text == "continue") tokenizer.curToken.kind = TokenKind::CONTINUE;
            }
            else if (IsNumeric(curChar)) {
                // integer literal
                tokenizer.curToken.kind = TokenKind::INTEGER_LITERAL;
                tokenizer.curToken.text = LeftChopWhile(tokenizer, IsNumeric);
                // float literal
                if (tokenizer.sourceIdx < tokenizer.source.size() && tokenizer.source[tokenizer.sourceIdx] == '.') {
                    tokenizer.curToken.kind = TokenKind::FLOAT_LITERAL;
                    ++tokenizer.sourceIdx;
                    std::string_view mantissa = LeftChopWhile(tokenizer, IsNumeric);
                    tokenizer.curToken.text = std::string_view{tokenizer.curToken.text.data(), tokenizer.curToken.text.size() + mantissa.size() + 1};
                }
            }
            else {
                return TokenUnimplemnted(std::string_view{&curChar, 1}); // FIXME: is this ub?
            }
        }
    }

    assert(tokenizer.curToken.kind != TokenKind::NONE);
    tokenizer.curToken.pos.line = tokenizer.curPos.line + 1;
    tokenizer.curToken.pos.col = tokenizer.curPos.col + 1;
    tokenizer.curPos.col += tokenizer.curToken.text.size();

    return { .err = TokenizerError::NONE };
}

TokenizerResult PollToken(Tokenizer& tokenizer) {
    TokenizerResult res = PollTokenWithComments(tokenizer);
    if (res.err != TokenizerError::NONE)
        return res;
    while (tokenizer.curToken.kind == TokenKind::COMMENT) {
        tokenizer.curToken.kind = TokenKind::NONE;
        res = PollTokenWithComments(tokenizer);
        if (res.err != TokenizerError::NONE)
            return res;
    }
    return res;
}

bool TokenizeOK(const TokenizerResult& result) {
    return result.err == TokenizerError::NONE;
}

void ConsumeToken(Tokenizer& tokenizer) {
    tokenizer.curToken.kind = TokenKind::NONE;
}



