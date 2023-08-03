#pragma once

#include "compileerror.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <fstream>

enum class TokenKind : uint8_t {
    NONE,
    COMMENT,
    SEMICOLON,
    IDENTIFIER,
    IF,
    ELSE,
    FOR,
    PROC,
    CDECL,
    EXTERN,
    PUBLIC,
    LET,
    MUT,
    RETURN,
    BREAK,
    CONTINUE,
    U8,
    I64,
    F64,
    OPERATOR_POS,
    OPERATOR_NEG,
    OPERATOR_MUL,
    OPERATOR_DIV,
    OPERATOR_MOD,
    OPERATOR_COMMA,
    OPERATOR_ASSIGN,
    OPERATOR_EQ,
    OPERATOR_NE,
    OPERATOR_GE,
    OPERATOR_GT,
    OPERATOR_LE,
    OPERATOR_LT,
    OPERATOR_NOT,
    OPERATOR_AND,
    OPERATOR_OR,
    LPAREN,
    RPAREN,
    LCURLY,
    RCURLY,
    LSQUARE,
    RSQUARE,
    ARROW,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    // ...
    TOKEN_COUNT
};
const char* TokenKindName(TokenKind kind);

struct FileLocation {
    size_t line, col, idx;
};

// TODO: use bitfield to eliminate wasted 7 bytes after kind
struct Token {
    const File* file;
    std::string_view text;
    FileLocation pos;
    TokenKind kind;

    friend struct fmt::formatter<Token>;
};

template<>
struct fmt::formatter<Token> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Token& token, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}:{}:\"{}\"",
            TokenKindName(token.kind),
            token.pos.line,
            token.pos.col,
            token.text);
    }
};


class Tokenizer {
    const File& file;
    FileLocation curPos{};
public:
    Token curToken{};

    explicit Tokenizer(const File& file_)
        : file{file_}
    {
        curToken.file = &file;
    }

    // Emits error and exits on error
    // Returns false when tokenizing EOF
    // Returns true otherwise (successfully got token)
    bool PollTokenWithComments();
    bool PollToken(); // Calls PollTokenWithComments
    void ConsumeToken();
};

std::vector<Token> TokenizeEntireSource(const std::vector<File>& files);

