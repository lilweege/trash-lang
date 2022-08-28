#include "tokenizer.hpp"
#include "compileerror.hpp"

#include <cassert>
#include <algorithm>
#include <array>
#include <functional>

const char* TokenKindName(TokenKind kind) {
    static_assert(static_cast<uint32_t>(TokenKind::TOKEN_COUNT) == 43, "Exhaustive check of token kinds failed");
    const std::array<const char*, static_cast<uint32_t>(TokenKind::TOKEN_COUNT)> TokenKindNames{
        "NONE",
        "COMMENT",
        "SEMICOLON",
        "IDENTIFIER",
        "IF",
        "ELSE",
        "FOR",
        "PROC",
        "LET",
        "MUT",
        "RETURN",
        "BREAK",
        "CONTINUE",
        "U8",
        "I64",
        "F64",
        "OPERATOR_POS",
        "OPERATOR_NEG",
        "OPERATOR_MUL",
        "OPERATOR_DIV",
        "OPERATOR_MOD",
        "OPERATOR_COMMA",
        "OPERATOR_ASSIGN",
        "OPERATOR_EQ",
        "OPERATOR_NE",
        "OPERATOR_GE",
        "OPERATOR_GT",
        "OPERATOR_LE",
        "OPERATOR_LT",
        "OPERATOR_NOT",
        "OPERATOR_AND",
        "OPERATOR_OR",
        "LPAREN",
        "RPAREN",
        "LCURLY",
        "RCURLY",
        "LSQUARE",
        "RSQUARE",
        "ARROW",
        "INTEGER_LITERAL",
        "FLOAT_LITERAL",
        "STRING_LITERAL",
        "CHAR_LITERAL",
    };
    return TokenKindNames[static_cast<uint32_t>(kind)];
}

static bool IsWhitespace(char ch) {
    return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
}

static void LeftTrim(std::string_view source, size_t& sourceIdx, size_t* outLineNo = nullptr, size_t* outColNo = nullptr) {
    for (;
        sourceIdx < source.size();
        ++sourceIdx)
    {
        char ch = source[sourceIdx];
        if (!IsWhitespace(ch)) {
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

static std::string_view LeftChop(std::string_view source, size_t& sourceIdx, size_t n) {
    std::string_view view{&source[sourceIdx], n};
    sourceIdx += n;
    return view;
}

static std::string_view LeftChopWhile(std::string_view source, size_t& sourceIdx, auto&& predicate) {
    // FIXME: narrowing conversion?
    const auto* begin = source.cbegin() + sourceIdx;
    const auto* end = std::find_if(begin, source.cend(), std::not_fn(predicate));
    sourceIdx += static_cast<size_t> (end - begin);
    return std::string_view{begin, end};
}

TokenizerResult Tokenizer::PollTokenWithComments() {
    if (curToken.kind != TokenKind::NONE) {
        return { .err = TokenizerError::NONE };
    }

    size_t lineDiff = 0, colDiff = 0;
    do {
        // whitespace
        size_t linesBefore = curPos.line;
        size_t colsBefore = curPos.col;
        LeftTrim(file.source, sourceIdx, &curPos.line, &curPos.col);

        // line comment beginning with '?'
        if (sourceIdx >= file.source.size()) {
            break;
        }
        char curChar = file.source[sourceIdx];
        if (curChar == '?') {
            // it is a comment
            curToken.kind = TokenKind::COMMENT;
            size_t commentEnd = file.source.find_first_of('\n', sourceIdx);
            if (commentEnd == std::string::npos) {
                // no newline implies end of file
                curToken.text = std::string_view{&file.source[sourceIdx]};
                curToken.pos.idx = sourceIdx;
                sourceIdx = file.source.size();

                curToken.pos.line = curPos.line + 1;
                curToken.pos.col = curPos.col + 1;
                curPos.col += curToken.text.size();
                return { .err = TokenizerError::NONE };
            }
            curToken.pos.idx = sourceIdx;
            curToken.text = LeftChop(file.source, sourceIdx, commentEnd-sourceIdx);
            curToken.pos.line = curPos.line + 1;
            curToken.pos.col = curPos.col + 1;
            curPos.col += curToken.text.size();
            return { .err = TokenizerError::NONE };
        }

        lineDiff = curPos.line - linesBefore;
        colDiff = curPos.col - colsBefore;
    } while (lineDiff != 0 || colDiff != 0);

    if (sourceIdx >= file.source.size()) {
        return { .err = TokenizerError::EMPTY };
    }

    auto TokenUnimplemnted = [&](std::string_view token) -> TokenizerResult {
        return {
            .err = TokenizerError::FAIL,
            .msg = CompileErrorMessage(
                file.filename,
                curPos.line + 1,
                curPos.col + 1,
                file.source, sourceIdx,
                "\"{}\" is not implemented yet", token)
        };
    };

    curToken.pos.idx = sourceIdx;
    char curChar = file.source[sourceIdx];
    switch (curChar) {
        case ';': {
            curToken.kind = TokenKind::SEMICOLON;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '(': {
            curToken.kind = TokenKind::LPAREN;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case ')': {
            curToken.kind = TokenKind::RPAREN;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '{': {
            curToken.kind = TokenKind::LCURLY;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '}': {
            curToken.kind = TokenKind::RCURLY;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '[': {
            curToken.kind = TokenKind::LSQUARE;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case ']': {
            curToken.kind = TokenKind::RSQUARE;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '+': {
            curToken.kind = TokenKind::OPERATOR_POS;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '-': {
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '>')
            {
                curToken.kind = TokenKind::ARROW;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_NEG;
                curToken.text = LeftChop(file.source, sourceIdx, 1);
            }
        } break;

        case '*': {
            curToken.kind = TokenKind::OPERATOR_MUL;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '/': {
            curToken.kind = TokenKind::OPERATOR_DIV;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '%': {
            curToken.kind = TokenKind::OPERATOR_MOD;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case ',': {
            curToken.kind = TokenKind::OPERATOR_COMMA;
            curToken.text = LeftChop(file.source, sourceIdx, 1);
        } break;

        case '=': {
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_EQ;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_ASSIGN;
                curToken.text = LeftChop(file.source, sourceIdx, 1);
            }
        } break;

        case '!': {
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_NE;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_NOT;
                curToken.text = LeftChop(file.source, sourceIdx, 1);
            }
        } break;

        case '&': {
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '&')
            {
                curToken.kind = TokenKind::OPERATOR_AND;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                // TODO: bitwise and
                return TokenUnimplemnted("&");
            }
        } break;

        case '|': {
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '|')
            {
                curToken.kind = TokenKind::OPERATOR_OR;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                // TODO: bitwise or
                return TokenUnimplemnted("|");
            }
        } break;

        case '>': {
            // TODO: right shift
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_GE;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_GT;
                curToken.text = LeftChop(file.source, sourceIdx, 1);
            }
        } break;

        case '<': {
            // TODO: left shift
            if (sourceIdx + 1 < file.source.size() &&
                file.source[sourceIdx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_LE;
                curToken.text = LeftChop(file.source, sourceIdx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_LT;
                curToken.text = LeftChop(file.source, sourceIdx, 1);
            }
        } break;

        case '\'':
        case '"': {
            char delim = LeftChop(file.source, sourceIdx, 1)[0];
            size_t idx = 0;
            FileLocation strStart = { .line = curPos.line + 1, .col = curPos.col + 1, };
            for (idx = sourceIdx;
                idx < file.source.size();
                ++idx)
            {
                if (file.source[idx] == delim) {
                    break;
                }
                if (file.source[idx] == '\n') {
                    return {
                        .err = TokenizerError::FAIL,
                        .msg = CompileErrorMessage(
                            file.filename, strStart.line, strStart.col,
                            file.source, sourceIdx,
                            "Unexpected end of line in string literal"),
                    };
                }
                if (file.source[idx] == '\\') {
                    if (++idx >= file.source.size()) {
                        break;
                    }
                    char escapeChar = file.source[idx];
                    if (escapeChar != delim && escapeChar != '\n' &&
                        escapeChar != 'n' && escapeChar != 't' && escapeChar != '\\')
                    {
                        return {
                            .err = TokenizerError::FAIL,
                            .msg = CompileErrorMessage(
                                file.filename,
                                curPos.line + 1,
                                curPos.col + idx - sourceIdx + 1,
                                file.source, sourceIdx,
                                "Invalid escape character \"\\{}\"", escapeChar),
                        };
                    }
                }
            }
            if (idx >= file.source.size()) {
                return {
                    .err = TokenizerError::FAIL,
                    .msg = CompileErrorMessage(
                        file.filename, strStart.line, strStart.col,
                        file.source, sourceIdx,
                        "Unexpected end of file in string literal"),
                };
            }
            curPos.col += 2; // quotes are not included in string literal token
            curToken.kind = delim == '"' ? TokenKind::STRING_LITERAL : TokenKind::CHAR_LITERAL;
            curToken.text = LeftChop(file.source, sourceIdx, idx-sourceIdx);
            if (curToken.kind == TokenKind::CHAR_LITERAL) {
                if ((curToken.text.size() == 2 && curToken.text[0] != '\\') ||
                    curToken.text.size() > 2)
                {
                    return {
                        .err = TokenizerError::FAIL,
                        .msg = CompileErrorMessage(
                            file.filename, strStart.line, strStart.col,
                            file.source, sourceIdx,
                            "Max length of character literal exceeded"),
                    };
                }
            }
            ++sourceIdx;
        } break;

        case '\\': return TokenUnimplemnted("\\");
        case '.': return TokenUnimplemnted(".");
        // TODO: other operators and combinations

        default: {
            auto IsIdentifier = [](char c) { return (isalnum(c) != 0) || c == '_'; };
            auto IsNumeric = [](char c) { return isdigit(c) != 0; };

            if ((isalpha(curChar) != 0) || curChar == '_') {
                // identifier or keyword
                curToken.kind = TokenKind::IDENTIFIER;
                curToken.text = LeftChopWhile(file.source, sourceIdx, IsIdentifier);
                if (curToken.text == "if")            curToken.kind = TokenKind::IF;
                else if (curToken.text == "else")     curToken.kind = TokenKind::ELSE;
                else if (curToken.text == "for")      curToken.kind = TokenKind::FOR;
                else if (curToken.text == "u8")       curToken.kind = TokenKind::U8;
                else if (curToken.text == "i64")      curToken.kind = TokenKind::I64;
                else if (curToken.text == "f64")      curToken.kind = TokenKind::F64;
                else if (curToken.text == "proc")     curToken.kind = TokenKind::PROC;
                else if (curToken.text == "let")      curToken.kind = TokenKind::LET;
                else if (curToken.text == "mut")      curToken.kind = TokenKind::MUT;
                else if (curToken.text == "return")   curToken.kind = TokenKind::RETURN;
                else if (curToken.text == "break")    curToken.kind = TokenKind::BREAK;
                else if (curToken.text == "continue") curToken.kind = TokenKind::CONTINUE;
            }
            else if (IsNumeric(curChar)) {
                // integer literal
                curToken.kind = TokenKind::INTEGER_LITERAL;
                curToken.text = LeftChopWhile(file.source, sourceIdx, IsNumeric);
                // float literal
                if (sourceIdx < file.source.size() && file.source[sourceIdx] == '.') {
                    curToken.kind = TokenKind::FLOAT_LITERAL;
                    ++sourceIdx;
                    std::string_view mantissa = LeftChopWhile(file.source, sourceIdx, IsNumeric);
                    curToken.text = std::string_view{curToken.text.data(), curToken.text.size() + mantissa.size() + 1};
                }
            }
            else {
                return TokenUnimplemnted(std::string_view{&curChar, 1}); // FIXME: is this ub?
            }
        }
    }

    assert(curToken.kind != TokenKind::NONE);
    curToken.pos.line = curPos.line + 1;
    curToken.pos.col = curPos.col + 1;
    curPos.col += curToken.text.size();

    return { .err = TokenizerError::NONE };
}

TokenizerResult Tokenizer::PollToken() {
    TokenizerResult res = PollTokenWithComments();
    if (res.err != TokenizerError::NONE)
        return res;
    while (curToken.kind == TokenKind::COMMENT) {
        curToken.kind = TokenKind::NONE;
        res = PollTokenWithComments();
        if (res.err != TokenizerError::NONE)
            return res;
    }
    return res;
}

void Tokenizer::ConsumeToken() {
    curToken.kind = TokenKind::NONE;
}

bool TokenizeOK(const TokenizerResult& result) {
    return result.err == TokenizerError::NONE;
}

void PrintToken(const Token& token, FILE* stream) {
    fmt::print(stream, "{}:{}:{}:\"{}\"", TokenKindName(token.kind), token.pos.line, token.pos.col, token.text);
}
