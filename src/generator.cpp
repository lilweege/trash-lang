#include "generator.hpp"
#include "compileerror.hpp"

#include <cassert>

void EmitInstructions(fmt::ostream& out, Target target, const std::vector<Instruction>& instructions) {
    if (target == Target::LINUX_NASM) {
        assert(0 && "Target LINUX_NASM is not implemented yet.");
    }
    if (target == Target::WINDOWS_NASM) {
        assert(0 && "Target WINDOWS_NASM is not implemented yet.");
    }
    if (target == Target::C) {
        assert(0 && "Target C is not implemented yet.");
    }

    out.print("...");

    assert(0);
}
