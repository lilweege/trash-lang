#include "tokenizer.hpp"
#include "compileerror.hpp"

#include <cassert>
#include <algorithm>
#include <array>
#include <functional>

const char* TokenKindName(TokenKind kind) {
    static_assert(static_cast<uint32_t>(TokenKind::TOKEN_COUNT) == 47, "Exhaustive check of token kinds failed");
    const std::array<const char*, static_cast<uint32_t>(TokenKind::TOKEN_COUNT)> TokenKindNames{
        "NONE",
        "COMMENT",
        "SEMICOLON",
        "IDENTIFIER",
        "IF",
        "ELSE",
        "FOR",
        "PROC",
        "CDECL",
        "EXTERN",
        "PUBLIC",
        "LET",
        "MUT",
        "RETURN",
        "BREAK",
        "CONTINUE",
        "ASM",
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

bool Tokenizer::PollTokenWithComments() {
    if (curToken.kind != TokenKind::NONE) {
        return true;
    }

    size_t lineDiff = 0, colDiff = 0;
    do {
        // whitespace
        size_t linesBefore = curPos.line;
        size_t colsBefore = curPos.col;
        LeftTrim(file.source, curPos.idx, &curPos.line, &curPos.col);

        // line comment beginning with '?'
        if (curPos.idx >= file.source.size()) {
            break;
        }
        char curChar = file.source[curPos.idx];
        if (curChar == '?') {
            // it is a comment
            curToken.kind = TokenKind::COMMENT;
            size_t commentEnd = file.source.find_first_of('\n', curPos.idx);
            if (commentEnd == std::string::npos) {
                // no newline implies end of file
                curToken.text = std::string_view{&file.source[curPos.idx]};
                curToken.pos.idx = curPos.idx;
                curPos.idx = file.source.size();

                curToken.pos.line = curPos.line + 1;
                curToken.pos.col = curPos.col + 1;
                curPos.col += curToken.text.size();
                return true;
            }
            curToken.pos.idx = curPos.idx;
            curToken.text = LeftChop(file.source, curPos.idx, commentEnd-curPos.idx);
            curToken.pos.line = curPos.line + 1;
            curToken.pos.col = curPos.col + 1;
            curPos.col += curToken.text.size();
            return true;
        }

        lineDiff = curPos.line - linesBefore;
        colDiff = curPos.col - colsBefore;
    } while (lineDiff != 0 || colDiff != 0);

    if (curPos.idx >= file.source.size()) {
        return false;
    }

    auto TokenUnimplemnted = [&](std::string_view tokName) -> bool {
        FileLocation pos{ curPos.line + 1, curPos.col + 1, curPos.idx };
        CompileErrorAtLocation(file, pos, "\"{}\" is not implemented yet", tokName);
        return false; // Unreachable
    };

    curToken.pos.idx = curPos.idx;
    char curChar = file.source[curPos.idx];
    switch (curChar) {
        case ';': {
            curToken.kind = TokenKind::SEMICOLON;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '(': {
            curToken.kind = TokenKind::LPAREN;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case ')': {
            curToken.kind = TokenKind::RPAREN;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '{': {
            curToken.kind = TokenKind::LCURLY;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '}': {
            curToken.kind = TokenKind::RCURLY;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '[': {
            curToken.kind = TokenKind::LSQUARE;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case ']': {
            curToken.kind = TokenKind::RSQUARE;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '+': {
            curToken.kind = TokenKind::OPERATOR_POS;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '-': {
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '>')
            {
                curToken.kind = TokenKind::ARROW;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_NEG;
                curToken.text = LeftChop(file.source, curPos.idx, 1);
            }
        } break;

        case '*': {
            curToken.kind = TokenKind::OPERATOR_MUL;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '/': {
            curToken.kind = TokenKind::OPERATOR_DIV;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '%': {
            curToken.kind = TokenKind::OPERATOR_MOD;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case ',': {
            curToken.kind = TokenKind::OPERATOR_COMMA;
            curToken.text = LeftChop(file.source, curPos.idx, 1);
        } break;

        case '=': {
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_EQ;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_ASSIGN;
                curToken.text = LeftChop(file.source, curPos.idx, 1);
            }
        } break;

        case '!': {
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_NE;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_NOT;
                curToken.text = LeftChop(file.source, curPos.idx, 1);
            }
        } break;

        case '&': {
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '&')
            {
                curToken.kind = TokenKind::OPERATOR_AND;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                // TODO: bitwise and
                return TokenUnimplemnted("&");
            }
        } break;

        case '|': {
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '|')
            {
                curToken.kind = TokenKind::OPERATOR_OR;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                // TODO: bitwise or
                return TokenUnimplemnted("|");
            }
        } break;

        case '>': {
            // TODO: right shift
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_GE;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_GT;
                curToken.text = LeftChop(file.source, curPos.idx, 1);
            }
        } break;

        case '<': {
            // TODO: left shift
            if (curPos.idx + 1 < file.source.size() &&
                file.source[curPos.idx + 1] == '=')
            {
                curToken.kind = TokenKind::OPERATOR_LE;
                curToken.text = LeftChop(file.source, curPos.idx, 2);
            }
            else {
                curToken.kind = TokenKind::OPERATOR_LT;
                curToken.text = LeftChop(file.source, curPos.idx, 1);
            }
        } break;

        case '\'':
        case '"': {
            char delim = LeftChop(file.source, curPos.idx, 1)[0];
            size_t idx = 0;
            FileLocation strStart = { .line = curPos.line + 1, .col = curPos.col + 1, .idx = curPos.idx };
            for (idx = curPos.idx;
                idx < file.source.size();
                ++idx)
            {
                if (file.source[idx] == delim) {
                    break;
                }
                if (file.source[idx] == '\n') {
                    CompileErrorAtLocation(file, strStart, "Unexpected end of line in string literal");
                    return false;
                }
                if (file.source[idx] == '\\') {
                    if (++idx >= file.source.size()) {
                        break;
                    }
                    char escapeChar = file.source[idx];
                    if (escapeChar != delim && escapeChar != '\n' &&
                        escapeChar != 'n' && escapeChar != 't' && escapeChar != '\\')
                    {
                        FileLocation pos{
                            .line = curPos.line + 1,
                            .col = curPos.col + idx - curPos.idx + 1,
                            .idx = curPos.idx
                        };
                        CompileErrorAtLocation(file, pos, "Invalid escape character \"\\{}\"", escapeChar);
                        return false;
                    }
                }
            }
            if (idx >= file.source.size()) {
                CompileErrorAtLocation(file, strStart, "Unexpected end of file in string literal");
                return false;
            }
            curPos.col += 2; // quotes are not included in string literal token
            curToken.kind = delim == '"' ? TokenKind::STRING_LITERAL : TokenKind::CHAR_LITERAL;
            curToken.text = LeftChop(file.source, curPos.idx, idx-curPos.idx);
            if (curToken.kind == TokenKind::CHAR_LITERAL) {
                if ((curToken.text.size() == 2 && curToken.text[0] != '\\') ||
                    curToken.text.size() > 2)
                {
                    CompileErrorAtLocation(file, strStart, "Max length of character literal exceeded");
                    return false;
                }
            }
            ++curPos.idx;
        } break;

        case '\\': return TokenUnimplemnted("\\");
        case '.': return TokenUnimplemnted(".");
        case '^': return TokenUnimplemnted("^");
        // TODO: pointers

        default: {
            auto IsIdentifier = [](char c) { return (isalnum(c) != 0) || c == '_'; };
            auto IsNumeric = [](char c) { return isdigit(c) != 0; };

            if ((isalpha(curChar) != 0) || curChar == '_') {
                // identifier or keyword
                curToken.text = LeftChopWhile(file.source, curPos.idx, IsIdentifier);
                if      (curToken.text == "if")       curToken.kind = TokenKind::IF;
                else if (curToken.text == "else")     curToken.kind = TokenKind::ELSE;
                else if (curToken.text == "for")      curToken.kind = TokenKind::FOR;
                else if (curToken.text == "u8")       curToken.kind = TokenKind::U8;
                else if (curToken.text == "i64")      curToken.kind = TokenKind::I64;
                else if (curToken.text == "f64")      curToken.kind = TokenKind::F64;
                else if (curToken.text == "proc")     curToken.kind = TokenKind::PROC;
                else if (curToken.text == "cdecl")    curToken.kind = TokenKind::CDECL;
                else if (curToken.text == "extern")   curToken.kind = TokenKind::EXTERN;
                else if (curToken.text == "public")   curToken.kind = TokenKind::PUBLIC;
                else if (curToken.text == "let")      curToken.kind = TokenKind::LET;
                else if (curToken.text == "mut")      curToken.kind = TokenKind::MUT;
                else if (curToken.text == "return")   curToken.kind = TokenKind::RETURN;
                else if (curToken.text == "break")    curToken.kind = TokenKind::BREAK;
                else if (curToken.text == "continue") curToken.kind = TokenKind::CONTINUE;
                else if (curToken.text == "asm")      curToken.kind = TokenKind::ASM;
                else                                  curToken.kind = TokenKind::IDENTIFIER;
            }
            else if (IsNumeric(curChar)) {
                // integer literal
                curToken.kind = TokenKind::INTEGER_LITERAL;
                curToken.text = LeftChopWhile(file.source, curPos.idx, IsNumeric);
                // float literal
                if (curPos.idx < file.source.size() && file.source[curPos.idx] == '.') {
                    curToken.kind = TokenKind::FLOAT_LITERAL;
                    ++curPos.idx;
                    std::string_view mantissa = LeftChopWhile(file.source, curPos.idx, IsNumeric);
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

    return true;
}

bool Tokenizer::PollToken() {
    bool ok = PollTokenWithComments();
    if (!ok) return ok;
    while (curToken.kind == TokenKind::COMMENT) {
        curToken.kind = TokenKind::NONE;
        ok = PollTokenWithComments();
        if (!ok) return ok;
    }
    return ok;
}

void Tokenizer::ConsumeToken() {
    curToken.kind = TokenKind::NONE;
}

std::vector<Token> TokenizeEntireSource(const std::vector<File>& files) {
    // It's debatable if it's better to tokenize the entire source and then parse all tokens,
    // or interleave tokenizing with parsing.
    // Notably, storing tokens contiguously and referring to them by pointer reduces the size of each AST node.
    // Also, constant context switching probably isn't good for the CPU.
    std::vector<Token> tokens;
    tokens.emplace_back(); // Reserved empty token

    for (const File& file : files) {
        for (Tokenizer tokenizer{file};
            tokenizer.PollToken();
            tokenizer.ConsumeToken())
        {
            tokens.push_back(tokenizer.curToken);
        }
    }

    return tokens;
}
