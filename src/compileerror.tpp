
#include <sstream>
#include <utility>

template<typename Arg, typename... Args>
static void OStreamArgs(std::ostream& os, Arg&& arg, Args&&... args) {
    os << std::forward<Arg>(arg);
    (os << ... << std::forward<Args>(args));
}

template<typename... Args>
std::string CompileErrorMessage(std::string_view filename, FileLocation location, Args&&... args) {
    std::ostringstream ss;
    OStreamArgs(ss, filename, ':', location.line, ':', location.col, ": error: ", std::forward<Args>(args)...);
    return ss.str();
}
