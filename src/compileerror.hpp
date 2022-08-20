#pragma once

#include "tokenizer.hpp"

#include <fmt/core.h>
#include <fmt/color.h>

#define CompileErrorMessage(filename, line, col, format, ...) \
    CompileErrorMessage_("{}:{}:{}: {}: " format, \
        fmt::styled(filename, fmt::emphasis::bold), \
        fmt::styled(line, fmt::emphasis::bold), \
        fmt::styled(col, fmt::emphasis::bold), \
        fmt::styled("Error", fmt::emphasis::bold | fg(fmt::color::red)) \
        __VA_OPT__(,) __VA_ARGS__)

template<typename S, typename... Args>
std::string CompileErrorMessage_(const S& format, Args&&... args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}
