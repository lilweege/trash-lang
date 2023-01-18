#pragma once

#include "parser.hpp"
#include "bytecode.hpp"
#include <fmt/os.h>

enum class Target {
    LINUX_NASM,
    WINDOWS_NASM,
    C,
};

void EmitInstructions(fmt::ostream& out, Target target, const std::vector<Instruction>& instructions);
