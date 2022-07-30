#pragma once

#include "tokenizer.hpp"

template<typename... Args>
std::string CompileErrorMessage(std::string_view filename, FileLocation location, Args&&... args);

#include "compileerror.tpp"
