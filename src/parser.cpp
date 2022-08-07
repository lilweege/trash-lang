#include "parser.hpp"
#include "tokenizer.hpp"

#include <iostream>

std::vector<AST> ParseProgram(Parser& parser) {
    std::vector<AST> ast;

    for (const Token& token : parser.tokens) {
        std::cout << TokenKindName(token.kind) << ":" <<
                token.pos.line << ":" <<
                token.pos.col << ":" <<
                " \"" << token.text << "\"\n";
    }

    std::cerr << sizeof(AST) << "\n";
    return ast;
}

