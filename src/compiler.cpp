#include "compileerror.hpp"
#include "compiler.hpp"
#include "analyzer.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "generator.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <fmt/core.h>
#include <fmt/os.h>


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

    if (opts.srcFn.empty()) {
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

    auto sz = static_cast<size_t>(sourceStream.tellg());
    if (sz == 0) return "";
    std::string contents;
    contents.resize(sz);
    sourceStream.seekg(0, std::ios::beg);
    sourceStream.read(&contents[0], // NOLINT (.data() is const)
        static_cast<long>(contents.size()));
    sourceStream.close();
    return contents;
}

void CompilerMain(int argc, char** argv) {
    assert(argc > 0);

    CompilerOptions options{ParseArguments(argc, argv)};
    std::string source = ReadEntireFile(options.srcFn);
    File file{options.srcFn, source};

    std::vector<Token> tokens = TokenizeEntireSource(file);
    AST ast = ParseEntireProgram(file, tokens);
    std::vector<Instruction> instructions = VerifyAST(file, tokens, ast);

    if (options.binFn.empty()) {
        InterpretInstructions(instructions);
    }
    else {
        fmt::ostream binFile = fmt::output_file(options.binFn);
        EmitInstructions(binFile, Target::X86_64_ELF, instructions);
        binFile.close();
    }

    fmt::print(stderr, "DONE!\n");
}
