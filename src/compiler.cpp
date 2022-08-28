#include "compileerror.hpp"
#include "compiler.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <fmt/core.h>


struct CompilerOptions {
    std::string srcFn;
    std::string binFn;
    // ...
};

static void PrintUsage() {
    std::vector<std::pair<char, std::string>> options;
    fmt::print(stderr,
        "Usage: trashc [options]\n"
        "-i <file>   The name of the input source file to be compiled.\n"
        "-o <file>   The name of the compiled output binary file.\n"
        "-h          Displays this information\n"
    );
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
    std::ifstream sourceStream{filename, std::ios::in | std::ios::binary | std::ios::ate};
    if (!sourceStream) {
        fmt::print(stderr, "Error: Could not open file \"{}\".\n", filename);
        exit(1);
    }
    std::string contents;
    contents.resize(sourceStream.tellg());
    sourceStream.seekg(0, std::ios::beg);
    sourceStream.read(&contents[0], contents.size());
    sourceStream.close();
    return contents;
}

static std::vector<Token> TokenizeEntireSource(Tokenizer& tokenizer) {
    // It's debatable if it's better to tokenize the entire source and then parse all tokens,
    // or interleave tokenizing with parsing.
    // Notably, storing tokens contiguously and referring to them by pointer reduces the size of each AST node.
    // Also, constant context switching probably isn't good for the CPU.
    std::vector<Token> tokens{};
    tokens.emplace_back(); // Reserved empty token

    while (true) {
        tokenizer.ConsumeToken();
        TokenizerResult result = tokenizer.PollToken();
        if (result.err == TokenizerError::FAIL) {
            fmt::print(stderr, "{}\n", result.msg);
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

    CompilerOptions options{ ParseArguments(argc, argv) };
    std::string source{ ReadEntireFile(options.srcFn) };
    File file{ options.srcFn, source };
    Tokenizer tokenizer{ file };
    Parser parser{ file, TokenizeEntireSource(tokenizer) };
    // { options.srcFn, source, std::move(parser.ast) };
    // TODO: typecheck and analyze
    // TODO: generate code
    fmt::print(stderr, "DONE!\n");
}
