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

enum class TokenizerError : uint8_t {
    NONE,
    EMPTY,
    FAIL,
};

struct TokenizerResult {
    TokenizerError err;
    std::string msg;
};

struct FileLocation {
    size_t line, col, idx;
};

struct Token {
    std::string_view text;
    FileLocation pos;
    TokenKind kind;
};

class Tokenizer {
public:
    const File file;
private:
    size_t sourceIdx{};
    FileLocation curPos{};
    std::vector<Token> tokens;

public:
    Token curToken;

    Tokenizer(File file_) : file{file_} {}

    TokenizerResult PollToken();
    TokenizerResult PollTokenWithComments();
    void ConsumeToken();
};

bool TokenizeOK(const TokenizerResult& result);
void PrintToken(const Token& token, FILE* stream = stderr);
