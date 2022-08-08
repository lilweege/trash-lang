#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class TokenKind {
    NONE,
    COMMENT,
    SEMICOLON,
    IDENTIFIER,
    IF,
    ELSE,
    FOR,
    PROC,
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

enum class TokenizerError {
    NONE,
    EMPTY,
    FAIL,
};

struct TokenizerResult {
    TokenizerError err;
    std::string msg;
};

struct FileLocation {
    uint32_t line, col;
};

struct Token {
    TokenKind kind;
    std::string_view text;
    FileLocation pos;
};

struct Tokenizer {
    std::string_view filename;
    std::string_view source;
    size_t sourceIdx;
    Token curToken;
    FileLocation curPos;
    std::vector<Token> tokens;
};


const char* TokenKindName(TokenKind kind);
TokenizerResult PollToken(Tokenizer& tokenizer);
void ConsumeToken(Tokenizer& tokenizer);
bool TokenizeOK(const TokenizerResult& result);
