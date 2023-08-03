#include "interpreter.hpp"
#include "compileerror.hpp"
#include "parser.hpp"

#include <cassert>
#include <unordered_map>

#define DBG_INS 0

char UnescapeChar(const char* buff, size_t sz) {
    if (sz == 0) return '\0';
    if (sz == 1) return buff[0];
    if (buff[0] == '\\') {
        char escapeChar = buff[1];
        switch (escapeChar) {
            case 'n': return '\n';
            case 't': return '\t';
            case '\\': return '\\';
            case '\'': return '\'';
            case '\"': return '\"';
        }
    }

    return buff[0];
}

std::string UnescapeString(const char* buff, size_t sz) {
    std::string str;
    str.reserve(sz);
    for (size_t i = 0; i < sz; ++i) {
        char c = UnescapeChar(buff+i, sz-i);
        if (buff[i] == '\\') ++i;
        str.push_back(c);
    }
    return str;
}

uint8_t st[10000000];
void InterpretInstructions(const std::vector<Procedure>& procedures) {
    // Flatten procedures to list of instructions
    std::vector<Instruction> instructions;
    for (const auto& proc : procedures) {
        instructions.insert(instructions.end(), proc.instructions.begin(), proc.instructions.end());
    }
    size_t sp = 0;
    size_t bp = sp;
    std::vector<std::string> stringLiteralPool;

    auto PrintStack = [&]() {
        assert(sp % 8 == 0);
        for (size_t i = 0; i < sp; i += 8) {
            for (size_t j = i; j < i+8; ++j) {
                fprintf(stderr, "%.2x", st[j]);
            }
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
    };

    for (size_t ip = 0; ip < instructions.size(); ++ip) {
        Instruction ins = instructions[ip];

#if DBG_INS
        fmt::print(stderr, "{:3}: ", ip);
#endif
        if (ins.opcode == Instruction::Opcode::PUSH) {
#if DBG_INS
                 if (ins.lit.kind == TypeKind::I64) fmt::print(stderr, "PUSH {}\n", ins.lit.i64);
            else if (ins.lit.kind == TypeKind::F64) fmt::print(stderr, "PUSH {}\n", ins.lit.f64);
            else if (ins.lit.kind == TypeKind::U8) fmt::print(stderr, "PUSH {}\n", (char)ins.lit.u8);
            else if (ins.lit.kind == TypeKind::STR) fmt::print(stderr, "PUSH \"{}\"\n", std::string_view{ins.lit.str.buf, ins.lit.str.sz});
            else assert(0);
#else
                 if (ins.lit.kind == TypeKind::I64) { memcpy(st + sp, &ins.lit.i64, 8); sp += 8; }
            else if (ins.lit.kind == TypeKind::F64) { memcpy(st + sp, &ins.lit.f64, 8); sp += 8; }
            else if (ins.lit.kind == TypeKind::U8)  { memcpy(st + sp, &ins.lit.u8, 1);  sp += 8; }
            else if (ins.lit.kind == TypeKind::STR) {
                int64_t poolIdx = stringLiteralPool.size();
                stringLiteralPool.emplace_back(UnescapeString(ins.lit.str.buf, ins.lit.str.sz));
                memcpy(st + sp, &poolIdx, 8);
                sp += 8;
            }
            else assert(0);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::LOAD_FAST) {
#if DBG_INS
            fmt::print(stderr, "LOAD_FAST {} ({})\n", ins.access.varAddr, ins.access.accessSize);
#else
            memcpy(st + sp, st + ins.access.varAddr * 8 + bp, ins.access.accessSize);
            sp += 8;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::STORE) {
#if DBG_INS
            fmt::print(stderr, "STORE ({})\n", ins.access.accessSize);
#else
            sp -= 8;
            void* addr = st + sp;
            int64_t offset = *(int64_t*) addr;
            sp -= 8;
            void* val = st + sp;
            memcpy(st + offset, val, ins.access.accessSize);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::STORE_FAST) {
#if DBG_INS
            fmt::print(stderr, "STORE_FAST {} ({})\n", ins.access.varAddr, ins.access.accessSize);
#else
            sp -= 8;
            memset(st + ins.access.varAddr * 8 + bp, 0, 8);
            memcpy(st + ins.access.varAddr * 8 + bp, st + sp, ins.access.accessSize);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::ALLOCA) {
#if DBG_INS
            fmt::print(stderr, "ALLOCA {}\n", ins.access.varAddr);
#else
            sp -= 8;
            void* addr = st + sp;
            int64_t offset = *(int64_t*) addr;
            memcpy(st + ins.access.varAddr * 8 + bp, &sp, 8);
            sp = (sp + offset + 7) & (-8);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::DEREF) {
#if DBG_INS
            fmt::print(stderr, "DEREF ({})\n", ins.access.accessSize);
#else
            int64_t x;
            sp -= 8;
            memcpy(&x, st + sp, 8);
            int64_t val = 0;
            memcpy(&val, st + x, ins.access.accessSize);
            memcpy(st + sp, &val, 8);
            sp += 8;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::UNARY_OP) {
#if DBG_INS
                 if (ins.op.op_kind == ASTKind::NEG_UNARYOP_EXPR) fmt::print(stderr, "UNOP (NEG {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::NOT_UNARYOP_EXPR) fmt::print(stderr, "UNOP (NOT {})\n", TypeKindName(ins.op.kind));
            else assert(0);
#else
            auto doOp = [&](auto& x) {
                sp -= 8;
                memcpy(&x, st + sp, sizeof(x));

                     if (ins.op.op_kind == ASTKind::NEG_UNARYOP_EXPR) x = -x;
                else if (ins.op.op_kind == ASTKind::NOT_UNARYOP_EXPR) x = !x;
                else assert(0);

                memcpy(st + sp, &x, sizeof(x));
                sp += 8;
            };

                 if (ins.op.kind == TypeKind::I64) { int64_t x; doOp(x); }
            else if (ins.op.kind == TypeKind::F64) { double x;  doOp(x); }
            else if (ins.op.kind == TypeKind::U8)  { uint8_t x; doOp(x); }
            else assert(0);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::BINARY_OP) {
#if DBG_INS
                 if (ins.op.op_kind == ASTKind::EQ_BINARYOP_EXPR) fmt::print(stderr, "BINOP (EQ {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::NE_BINARYOP_EXPR) fmt::print(stderr, "BINOP (NE {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::GE_BINARYOP_EXPR) fmt::print(stderr, "BINOP (GE {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::GT_BINARYOP_EXPR) fmt::print(stderr, "BINOP (GT {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::LE_BINARYOP_EXPR) fmt::print(stderr, "BINOP (LE {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::LT_BINARYOP_EXPR) fmt::print(stderr, "BINOP (LT {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::AND_BINARYOP_EXPR) fmt::print(stderr, "BINOP (AND {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::OR_BINARYOP_EXPR) fmt::print(stderr, "BINOP (OR {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::ADD_BINARYOP_EXPR) fmt::print(stderr, "BINOP (ADD {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::SUB_BINARYOP_EXPR) fmt::print(stderr, "BINOP (SUB {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::MUL_BINARYOP_EXPR) fmt::print(stderr, "BINOP (MUL {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::DIV_BINARYOP_EXPR) fmt::print(stderr, "BINOP (DIV {})\n", TypeKindName(ins.op.kind));
            else if (ins.op.op_kind == ASTKind::MOD_BINARYOP_EXPR) fmt::print(stderr, "BINOP (MOD {})\n", TypeKindName(ins.op.kind));
            else assert(0);
#else
            auto doOp = [&]<typename T>(T& x, T& y) {
                sp -= 8;
                memcpy(&x, st + sp, sizeof(x));
                sp -= 8;
                memcpy(&y, st + sp, sizeof(y));
                
                memset(st + sp, 0, 8);
                bool isLogical =
                    ins.op.op_kind == ASTKind::EQ_BINARYOP_EXPR ||
                    ins.op.op_kind == ASTKind::NE_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::GE_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::GT_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::LE_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::LT_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::AND_BINARYOP_EXPR || 
                    ins.op.op_kind == ASTKind::OR_BINARYOP_EXPR;
                if (isLogical) {
                    uint8_t z;
                         if (ins.op.op_kind == ASTKind::EQ_BINARYOP_EXPR)  z = bool(y == x);
                    else if (ins.op.op_kind == ASTKind::NE_BINARYOP_EXPR)  z = bool(y != x);
                    else if (ins.op.op_kind == ASTKind::GE_BINARYOP_EXPR)  z = bool(y >= x);
                    else if (ins.op.op_kind == ASTKind::GT_BINARYOP_EXPR)  z = bool(y >  x);
                    else if (ins.op.op_kind == ASTKind::LE_BINARYOP_EXPR)  z = bool(y <= x);
                    else if (ins.op.op_kind == ASTKind::LT_BINARYOP_EXPR)  z = bool(y <  x);
                    else if (ins.op.op_kind == ASTKind::AND_BINARYOP_EXPR) z = bool(y && x);
                    else if (ins.op.op_kind == ASTKind::OR_BINARYOP_EXPR)  z = bool(y || x);
                    else assert(0);
                    memcpy(st + sp, &z, 1);
                }
                else {
                         if (ins.op.op_kind == ASTKind::ADD_BINARYOP_EXPR) y += x;
                    else if (ins.op.op_kind == ASTKind::SUB_BINARYOP_EXPR) y -= x;
                    else if (ins.op.op_kind == ASTKind::MUL_BINARYOP_EXPR) y *= x;
                    else if (ins.op.op_kind == ASTKind::DIV_BINARYOP_EXPR) y /= x;
                    else if constexpr (std::is_integral_v<T>) {
                        if (ins.op.op_kind == ASTKind::MOD_BINARYOP_EXPR) y %= x;
                    }
                    else assert(0);
                    memcpy(st + sp, &y, sizeof(y));
                }
                sp += 8;
            };

                 if (ins.op.kind == TypeKind::I64) { int64_t x, y; doOp(x, y); }
            else if (ins.op.kind == TypeKind::F64) { double x, y;  doOp(x, y); }
            else if (ins.op.kind == TypeKind::U8)  { uint8_t x, y; doOp(x, y); }
            else assert(0);
#endif
        }
        else if (ins.opcode == Instruction::Opcode::JMP) {
            if (IS_BUILTIN(ins.jmpAddr)) {
#if DBG_INS
                fmt::print(stderr, "CALL {}\n", (int64_t) ins.jmpAddr);
#else
                if (ins.jmpAddr == BUILTIN_putf) {
                    double x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    fmt::print(stdout, "{}", x);
                    // printf("0x%x\n", x);
                }
                else if (ins.jmpAddr == BUILTIN_puti) {
                    int64_t x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    fmt::print(stdout, "{}", x);
                    // printf("0x%x\n", x);
                }
                else if (ins.jmpAddr == BUILTIN_puts) {
                    int64_t idx;
                    sp -= 8;
                    memcpy(&idx, st + sp, 8);
                    sp -= 16;
                    fmt::print(stdout, "{}", stringLiteralPool[idx]);
                }
                else if (ins.jmpAddr == BUILTIN_itoc) {
                    int64_t x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    x &= 0xFF;
                    memcpy(st + sp, &x, 8);
                    sp += 8;
                }
                else if (ins.jmpAddr == BUILTIN_ctoi) {
                    int64_t x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    memcpy(st + sp, &x, 8);
                    sp += 8;
                }
                else if (ins.jmpAddr == BUILTIN_itof) {
                    int64_t x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    double y = (double) x;
                    memcpy(st + sp, &y, 8);
                    sp += 8;
                }
                else if (ins.jmpAddr == BUILTIN_ftoi) {
                    double x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    int64_t y = (int64_t) x;
                    memcpy(st + sp, &y, 8);
                    sp += 8;
                }
                else if (ins.jmpAddr == BUILTIN_sqrt) {
                    double x;
                    sp -= 8;
                    memcpy(&x, st + sp, 8);
                    sp -= 16;
                    x = sqrtf(x);
                    memcpy(st + sp, &x, 8);
                    sp += 8;
                }
                else {
                    fmt::print(stderr, "{}\n", (int64_t) ins.jmpAddr);
                    assert(0);
                }
#endif
            }
            else {
#if DBG_INS
                fmt::print(stderr, "JMP {}\n", ins.jmpAddr);
#else
                ip = ins.jmpAddr - 1;
#endif
            }
        }
        else if (ins.opcode == Instruction::Opcode::JMP_NZ) {
#if DBG_INS
            fmt::print(stderr, "JMP_NZ {}\n", ins.jmpAddr);
#else
            int64_t x;
            sp -= 8;
            memcpy(&x, st + sp, 8);
            if (x) ip = ins.jmpAddr - 1;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::JMP_Z) {
#if DBG_INS
            fmt::print(stderr, "JMP_Z {}\n", ins.jmpAddr);
#else
            int64_t x;
            sp -= 8;
            memcpy(&x, st + sp, 8);
            if (!x) ip = ins.jmpAddr - 1;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::SAVE) {
#if DBG_INS
            fmt::print(stderr, "SAVE {}\n", ins.jmpAddr);
#else
            memcpy(st + sp, &ins.jmpAddr, 8);
            sp += 8;
            memcpy(st + sp, &bp, 8);
            sp += 8;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::ENTER) {
#if DBG_INS
            fmt::print(stderr, "ENTER {} {}\n", ins.frame.numParams, ins.frame.numLocals);
#else
            bp = sp - ins.frame.numParams * 8;
            sp += ins.frame.numLocals * 8;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::RETURN_VOID) {
#if DBG_INS
            fmt::print(stderr, "RETURN_VOID\n");
#else
            if (bp == 0) break;
            // sp = bp
            sp = bp;
            // bp = TOP
            sp -= 8;
            memcpy(&bp, st + sp, 8);
            // jmp TOP
            int64_t retAddr;
            sp -= 8;
            memcpy(&retAddr, st + sp, 8);
            ip = retAddr - 1;
#endif
        }
        else if (ins.opcode == Instruction::Opcode::RETURN_VAL) {
#if DBG_INS
            fmt::print(stderr, "RETURN_VAL\n");
#else
            if (bp == 0) break;
            // tmp = TOP
            int64_t tmp;
            sp -= 8;
            memcpy(&tmp, st + sp, 8);
            // sp = bp
            sp = bp;
            // bp = TOP
            sp -= 8;
            memcpy(&bp, st + sp, 8);
            // TOP = tmp
            memcpy(st + sp, &tmp, 8);
            sp += 8;
            // swap
            memcpy(&tmp, st + sp - 8, 8);          // tmp = TOP
            memcpy(st + sp - 8, st + sp - 16, 8);  // TOP = TOP1
            memcpy(st + sp - 16, &tmp, 8);         // TOP1 = tmp
            // jmp TOP
            int64_t retAddr;
            sp -= 8;
            memcpy(&retAddr, st + sp, 8);
            ip = retAddr - 1;
#endif
        }
        else assert(0);

#if DBG_INS
#else
        // fmt::print(stderr, "ip={} bp={} sp={}\n", ip, bp, sp);
        // PrintStack();
        // char s[100];
        // gets(s);
#endif
    }
}
