#pragma once

#include "tokenizer.hpp"

#include <fmt/core.h>
#include <fmt/color.h>

static bool IsNewline(char ch) {
    return ch == '\r' || ch == '\n';
}
static bool IsWhitespace(char ch) {
    return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
}

#define CompileErrorMessage(filename, line, col, source, sourceIdx, format, ...) \
    CompileErrorMessage_("{}:{}:{}: {}" format "\n{}\n{:>{}}", \
        fmt::styled(filename, fmt::emphasis::bold), \
        fmt::styled(line, fmt::emphasis::bold), \
        fmt::styled(col, fmt::emphasis::bold), \
        fmt::styled("error: ", fmt::emphasis::bold | fg(fmt::terminal_color::bright_red)), \
        __VA_ARGS__ __VA_OPT__(,) \
        ExtractWholeLine(source, sourceIdx), \
        fmt::styled("^", fg(fmt::terminal_color::bright_green)), col \
    )

static std::string_view ExtractWholeLine(std::string_view source, size_t sourceIdx) {
    auto lineStart = source.begin() + sourceIdx - 1;
    auto lineEnd = source.begin() + sourceIdx;
    while (lineStart != source.begin() && !IsNewline(*lineStart)) {
        --lineStart;
    }
    if (IsNewline(*lineStart)) ++lineStart;
    while (lineEnd != source.end() && !IsNewline(*lineEnd)) {
        ++lineEnd;
    }
    return std::string_view{lineStart, lineEnd};
}

template<typename S, typename... Args>
std::string CompileErrorMessage_(const S& format, Args&&... args) {
    return fmt::vformat(format, fmt::make_format_args(args...));
}
