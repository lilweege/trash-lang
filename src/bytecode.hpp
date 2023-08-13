#pragma once

#include "parser.hpp"

// Builtin functions have these arbitrary addresses
// The addresses are generation-dependant
#define BUILTIN_sqrt ((size_t)(-1))
#define BUILTIN_puti ((size_t)(-2))
#define BUILTIN_putf ((size_t)(-3))
#define BUILTIN_puts ((size_t)(-4))
#define IS_BUILTIN(x) ((int64_t)(x) < 0)

enum class LiteralKind {
    U8,
    I64,
    F64,
    STR,
    // ...
    COUNT,
};

enum class AccessKind {
    INTEGRAL, // Includes pointers
    FLOATING,
};

struct Instruction {
    enum class Opcode {
        // NOP,
        INLINE,      // Inline asm
        PUSH,        // TOP = x
        LOAD_FAST,   // TOP = *x
        STORE_FAST,  // *x = TOP
        LOAD,        // TOP = *(TOP + [TOP*x] + y)
        STORE,       // *(TOP + [TOP*x] + y) = TOP1
        ALLOCA,      // TOP = alloca(TOP*x)
        UNARY_OP,    // TOP = u(x, TOP)
        BINARY_OP,   // TOP = b(x, TOP1, TOP)
        CALL,        // call extern func
        JMP,         // ip = x
        JMP_Z,       // if TOP == 0: ip = x

        SAVE,        // TOP = ip; TOP = sp
        ENTER,       // bp = sp - x; sp += y
        RETURN_VOID, // sp = bp; bp = TOP; jmp TOP
        RETURN_VAL,  // tmp = TOP; sp = bp; bp = TOP; TOP = tmp; swap; jmp TOP
    } opcode;

    struct Literal {
        LiteralKind kind_;
        union {
            uint64_t i64;
            double f64;
            uint8_t u8;
            ASTNode::StringView str;
        };
    };

    struct Operator {
        ASTKind op_kind;
        AccessKind accessKind;
    };

    struct ConditionalJump {
        size_t jmpAddr;
        size_t accessSize;
    };

    struct StackFrame {
        size_t numParams;
        size_t localStackSize;
    };

    struct Access {
        size_t accessSize;
        size_t offset;
        bool hasSubscript;
    };

    struct FastAccess {
        size_t varAddr;
        size_t accessSize;
    };

    union {
        ASTNode::StringView str; // call, inline
        Literal lit; // push
        Access access; // load, store
        FastAccess fastAccess; // load_fast, store_fast
        Operator op; // unaryop, binaryop
        ConditionalJump jmp; // jz
        uint64_t jmpAddr; // jmp, save
        uint64_t multiplier; // alloca
        StackFrame frame; // enter, return
    };
};
