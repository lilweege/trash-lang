#pragma once

#include "analyzer.hpp"
#include "bytecode.hpp"
#include <vector>


char UnescapeChar(const char* buff, size_t sz);
std::string UnescapeString(const char* buff, size_t sz);

void InterpretInstructions(const std::vector<Procedure>& procedures);
