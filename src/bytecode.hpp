#pragma once

#include "parser.hpp"

// Builtin functions have these arbitrary addresses
// The addresses are generation-dependant
#define BUILTIN_sqrt ((size_t)(-1))
#define BUILTIN_puti ((size_t)(-2))
#define BUILTIN_putf ((size_t)(-3))
#define BUILTIN_puts ((size_t)(-4))
#define BUILTIN_itof ((size_t)(-5))
#define BUILTIN_ftoi ((size_t)(-6))
#define BUILTIN_itoc ((size_t)(-7))
#define BUILTIN_ctoi ((size_t)(-8))
#define IS_BUILTIN(x) ((int64_t)(x) < 0)

struct Instruction {
    enum class Opcode {
        // NOP,
        PUSH,        // TOP = x
        LOAD_FAST,   // TOP = *x
        STORE,       // *TOP = TOP1
        STORE_FAST,  // *x = TOP
        ALLOCA,      // *x = alloca(TOP)
        DEREF,       // TOP = *TOP
        UNARY_OP,    // TOP = u(x, TOP)
        BINARY_OP,   // TOP = b(x, TOP1, TOP)
        JMP,         // ip = x
        JMP_NZ,      // if TOP != 0: ip = x
        JMP_Z,       // if TOP == 0: ip = x

        SAVE,        // TOP = sp
        ENTER,       // bp = sp - x; sp += y
        RETURN_VOID, // sp = bp; bp = TOP; jmp TOP
        RETURN_VAL,  // tmp = TOP; sp = bp; bp = TOP; TOP = tmp; swap; jmp TOP
    } opcode;

    struct Literal {
        TypeKind kind;
        union {
            uint64_t i64;
            double f64;
            uint8_t u8;
            ASTNode::StringView str;
        };
    };

    struct Operator {
        TypeKind kind;
        ASTKind op_kind;
    };

    struct StackFrame {
        size_t numParams;
        size_t numLocals;
    };

    union {
        Literal lit; // push
        size_t varAddr; // load_fast, store_fast, alloca
        Operator op; // unaryop, binaryop
        uint64_t jmpAddr; // jmp, jz, jnz
        StackFrame frame; // enter, return
    };
};
