#include "compiler.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>


struct CompilerOptions {
    std::string srcFn;
    std::string binFn;
    // ...
};

struct Compiler {
    CompilerOptions options;
    std::string source;
    std::vector<Token> tokens;
    std::vector<AST> ast; // NOTE: these are not pointers, nodes live here
};

static void PrintUsage() {
    std::vector<std::pair<char, std::string>> options;
    std::cerr <<
        "Usage: trashc [options]\n"
        "-i <file>   The name of the input source file to be compiled.\n"
        "-o <file>   The name of the compiled output binary file.\n"
        "-h          Displays this information\n"
    ;
}

static CompilerOptions ParseArguments(int argc, char** argv) {
    CompilerOptions opts{};
    std::vector<std::string> args(argv+1, argv+argc);
    for (auto it = begin(args); it != end(args); ++it) {
        const std::string& arg = *it;
        if (arg == "-h") {
            PrintUsage();
            exit(0);
        }

        if (++it == cend(args)) {
            PrintUsage();
            exit(1);
        }

        if (arg == "-i") {
            opts.srcFn = std::move(*it);
        }
        else if (arg == "-o") {
            opts.binFn = std::move(*it);
        }
        else {
            PrintUsage();
            exit(1);
        }
    }

    if (opts.srcFn.empty() || opts.binFn.empty()) {
        PrintUsage();
        exit(1);
    }
    return opts;
}

static std::string ReadEntireFile(const std::string& filename) {
    std::ifstream sourceStream{filename, std::ios::in | std::ios::binary};
    if (!sourceStream) {
        std::cerr << "Error: Could not open file \"" << filename << "\".\n";
        exit(1);
    }

    return std::string{std::istreambuf_iterator<char>{sourceStream}, {}};
}

static std::vector<Token> TokenizeEntireSource(Tokenizer& tokenizer) {
    // It's debatable if it's better to tokenize the entire source and then parse all tokens,
    // or interleave tokenizing with parsing.
    // Notably, storing tokens contiguously and referring to them by pointer reduces the size of each AST node.
    // Also, constant context switching probably isn't good for the CPU.
    std::vector<Token> tokens{};
    tokens.emplace_back(); // Reserved empty token

    while (true) {
        ConsumeToken(tokenizer);
        TokenizerResult result = PollToken(tokenizer);
        if (result.err == TokenizerError::FAIL) {
            std::cerr << result.msg << '\n';
            exit(1);
        }
        else if (result.err == TokenizerError::EMPTY) {
            break;
        }
        tokens.push_back(tokenizer.curToken);
    }

    return tokens;
}


void CompilerMain(int argc, char** argv) {
    assert(argc > 0);

    Compiler compiler{};
    compiler.options = ParseArguments(argc, argv);
    compiler.source = ReadEntireFile(compiler.options.srcFn);
    Tokenizer tokenizer{ .filename = compiler.options.srcFn, .source = compiler.source };
    Parser parser{ .filename = compiler.options.srcFn, .tokens = TokenizeEntireSource(tokenizer) };
    compiler.ast = ParseEntireProgram(parser);
    std::cerr << "DONE!\n";
}

