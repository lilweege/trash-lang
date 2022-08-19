#pragma once

#include "tokenizer.hpp"

#include <fmt/core.h>

#define CompileErrorMessage(filename, line, col, fmt, ...) \
    CompileErrorMessage_("{}:{}:{}: Error: " fmt, filename, line, col __VA_OPT__(,) __VA_ARGS__)

template<typename S, typename... Args>
std::string CompileErrorMessage_(const S& fmt, Args&&... args) {
    return fmt::vformat(fmt, fmt::make_format_args(args...));
}
