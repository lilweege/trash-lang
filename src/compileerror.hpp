#pragma once

#include "tokenizer.hpp"

#include <sstream>
#include <utility>

template<typename... Args>
std::string CompileErrorMessage(std::string_view filename, size_t line, size_t col, Args&&... args) {
    std::ostringstream ss;
    // TODO: colored messages
    ss << filename << ':' << line << ':' << col << ": error: ";
    (ss << ... << std::forward<Args>(args));
    return ss.str();
}

