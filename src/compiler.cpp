#include "compileerror.hpp"
#include "compiler.hpp"
#include "analyzer.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include "generator.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <fmt/core.h>
#include <fmt/os.h>


struct CompilerOptions {
    std::vector<std::string> srcFn;
    std::string binFn;
    // ...
};

static void PrintUsage() {
    std::vector<std::pair<char, std::string>> options;
    fmt::print(stderr,
        "Usage: trashc [options]\n"
        "-i <files>   The name(s) of the input source file(s) to be compiled.\n"
        "-o <file>    The name of the compiled output binary file.\n"
        "-h           Displays this information\n"
    );
}

static CompilerOptions ParseArguments(int argc, char** argv) {
    CompilerOptions opts{};
    std::vector<std::string> args(argv+1, argv+argc);

    enum class Reading { None, Input, Output };
    Reading current = Reading::None;

    for (auto it = cbegin(args); it != cend(args); ++it) {
        const std::string& arg = *it;
        if (arg == "-h") {
            PrintUsage();
            exit(0);
        }
        else if (arg == "-i") {
            current = Reading::Input;
            if (it+1 == cend(args)) {
                PrintUsage();
                exit(1);
            }
        }
        else if (arg == "-o") {
            current = Reading::Output;
            if (it+1 == cend(args)) {
                PrintUsage();
                exit(1);
            }
        }
        else if (current == Reading::Input) {
            opts.srcFn.emplace_back(std::move(*it));
        }
        else if (current == Reading::Output) {
            opts.binFn = std::move(*it);
        }
        else {
            fmt::print("bad\n");
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

    std::vector<std::string> sources;
    for (const auto& fn : options.srcFn)
        sources.emplace_back(ReadEntireFile(fn));
    std::vector<File> files;
    for (size_t i = 0; i < sources.size(); ++i)
         files.push_back(File{options.srcFn[i], sources[i]});

    std::vector<Token> tokens = TokenizeEntireSource(files);
    AST ast = ParseEntireProgram(tokens);
    std::vector<Procedure> procedures = VerifyAST(tokens, ast);

    if (options.binFn.empty()) {
        assert(0);
    }
    else {
        fmt::ostream binFile = fmt::output_file(options.binFn);
        EmitInstructions(binFile, Target::X86_64_ELF, procedures);
        binFile.close();
    }

    fmt::print(stderr, "DONE!\n");
}
