#pragma once

#include "bytecode.hpp"
#include <vector>

void InterpretInstructions(const std::vector<Instruction>& instructions);
