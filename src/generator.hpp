#pragma once

#include "analyzer.hpp"
#include "parser.hpp"
#include "bytecode.hpp"
#include <fmt/os.h>

enum class Target {
    X86_64_ELF,
    POSIX_C,
    // WINDOWS_NASM,
};

void EmitInstructions(fmt::ostream& out, Target target, const std::vector<Procedure>& procedures);
