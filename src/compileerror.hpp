#pragma once

#include <string_view>
#include <fmt/core.h>
#include <fmt/color.h>

static bool IsNewline(char ch) {
    return ch == '\r' || ch == '\n';
}

struct File {
    std::string_view filename;
    std::string_view source;
};


// Convenient when "file" is local or member
#define CompileErrorAt(token, format, ...) CompileErrorAtToken(file, token, format, __VA_ARGS__)

#define CompileErrorAtToken(file, token, format, ...) CompileErrorAtLocation(file, (token).pos, format, __VA_ARGS__)

#define CompileErrorAtLocation(file, pos, format, ...) do { \
        fmt::print(stderr, "{}\n", CompileErrorMessage((file).filename, (pos).line, (pos).col, \
            (file).source, (pos).idx, format __VA_OPT__(,) __VA_ARGS__)); \
        exit(1); \
    } while (0)

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

static inline std::string_view ExtractWholeLine(std::string_view source, size_t sourceIdx) {
    const auto *lineStart = source.begin() + sourceIdx - 1;
    const auto *lineEnd = source.begin() + sourceIdx;
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
