#include "compiler.hpp"
#include "tokenizer.hpp"

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

static void PrintUsage() {
    std::vector<std::pair<char, std::string>> options;
    std::cerr <<
        "Usage: trashc [options]\n"
        "-i <file>   The name of the input source file to be compiled.\n"
        "-o <file>   The name of the compiled output binary file.\n"
        "-h          Displays this information\n"
    ;
}

CompilerOptions ParseArguments(int argc, char** argv) {
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
    return opts;
}

static std::string ReadFile(const std::string& filename) {
    std::ifstream sourceStream{filename, std::ios::in | std::ios::binary};
    if (!sourceStream) {
        std::cerr << "Error: Could not open file \"" << filename << "\".\n";
        exit(1);
    }

    return std::string{std::istreambuf_iterator<char>{sourceStream}, {}}; 
}

void CompilerMain(int argc, char** argv) {
    assert(argc > 0);
    const CompilerOptions opts = ParseArguments(argc, argv);

    if (opts.srcFn.empty() ||
        opts.binFn.empty())
    {
        PrintUsage();
        exit(1);
    }
    
    Tokenizer tokenizer{
        .filename = opts.srcFn,
        .source = ReadFile(opts.srcFn),
    };

    while (true) {
        TokenizerResult r = PollToken(tokenizer);
        if (r.err == TokenizerError::FAIL) {
            std::cerr << r.msg << '\n';
            exit(1);
        }
        if (r.err == TokenizerError::EMPTY) {
            break;
        }
        std::cout << TokenKindName(tokenizer.curToken.kind) << ":" <<
                tokenizer.curToken.pos.line << ":" <<
                tokenizer.curToken.pos.col << ":" <<
                " \"" << tokenizer.curToken.text << "\"\n";
        tokenizer.curToken.kind = TokenKind::NONE;
    }
}
