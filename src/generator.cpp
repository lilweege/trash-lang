#include "generator.hpp"
#include "bytecode.hpp"
#include "compileerror.hpp"
#include "interpreter.hpp"

#include <cassert>
#define DBG_INS 1

void EmitInstructions(fmt::ostream& out, Target target, const std::vector<Procedure>& procedures) {
    if (target == Target::X86_64_ELF) {
        std::vector<std::string> stringLiteralPool;
        std::vector<double> floatLiteralPool;
        // std::vector<uint16_t> ehdr = {
        //     0x7f45, 0x4c46, 0x0201, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000,
        //     0x0100, 0x3e00, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        //     0x0000, 0x0000, 0x0000, 0x0000, 0x6001, 0x0000, 0x0000, 0x0000,
        //     0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x4000, 0x0900, 0x0100 };
        // for (auto& x : ehdr) x = (x>>8) | (x<<8);
        // auto sv = std::string_view{(const char*)ehdr.data(), ehdr.size()*2};

        out.print(
            "BITS 64\n"
            "section .text\n"
        );
        for (const auto& proc : procedures) {
            if (proc.procInfo.isPublic) {
                out.print("global {}\n", proc.procName); // sysv abi
                out.print("global trash_{}\n", proc.procName);
            }
            else if (proc.procInfo.isExtern) {
                out.print("extern {}\n", proc.procName);
            }
        }
        for (const auto& proc : procedures) {
            if (proc.procInfo.isPublic) {
                out.print("{}:\n", proc.procName);
                out.print("push rbp\n");
                size_t numInts = 0;
                size_t numFloats = 0;
                for (ASTNode::ASTDefinition param : proc.params) {
                    TypeKind kind = param.type;
                    bool isPointer = param.arraySize != 0;
                    if (isPointer || kind == TypeKind::I64 || kind == TypeKind::U8) {
                        switch (numInts++) {
                            case 0: out.print("push rdi\n"); break;
                            case 1: out.print("push rsi\n"); break;
                            case 2: out.print("push rdx\n"); break;
                            case 3: out.print("push rcx\n"); break;
                            case 4: out.print("push r8\n"); break;
                            case 5: out.print("push r9\n"); break;
                            default: assert(0 && "TODO");//out.print("push [rsp+{}]\n", ); break;
                        }
                    }
                    else if (kind == TypeKind::F64) {
                        switch (numFloats++) {
                            case 0: out.print("movq r10, xmm0\n"); break;
                            case 1: out.print("movq r10, xmm1\n"); break;
                            case 2: out.print("movq r10, xmm2\n"); break;
                            case 3: out.print("movq r10, xmm3\n"); break;
                            case 4: out.print("movq r10, xmm4\n"); break;
                            case 5: out.print("movq r10, xmm5\n"); break;
                            case 6: out.print("movq r10, xmm6\n"); break;
                            case 7: out.print("movq r10, xmm7\n"); break;
                            default: assert(0 && "TODO");//out.print("push [rsp+{}]\n", ); break;
                        }
                        out.print("push r10\n");
                    }
                    else assert(0);
                }
                out.print("jmp trash_{}\n", proc.procName);
                if (proc.procInfo.retType != TypeKind::NONE) {
                    out.print("pop rax\n", proc.procName);
                }
            }
            else if (proc.procInfo.isExtern) {
                // TODO: Parameters and return value
                out.print("extern_{0}:\n"
                          "mov rbp, rsp\n"
                          "call {0}\n"
                          "mov rsp, rbp\n"
                          "pop rbp\n"
                          "ret\n",
                          proc.procName);
            }
        }

        // x/8gx $rbp
        out.print("BUILTIN_puts:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "mov rdi, [rbp-8]\n"
                  "call putcs\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "ret\n"
        );
        out.print("BUILTIN_puti:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "mov rdi, [rbp-8]\n"
                  "call puti\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "ret\n"
        );
        out.print("BUILTIN_putf:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "movsd xmm0, [rbp-8]\n"
                  "mov rdi, 8\n" // precision - TODO: Make optional parameter
                  "call putf\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "ret\n"
        );
        out.print("BUILTIN_ctoi:\n"
                  "BUILTIN_itoc:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "movzx eax, BYTE [rbp-8]\n"
                  "push rax\n"
                  "pop rax\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "push rax\n"
                  "pop rax\n"
                  "pop rbx\n"
                  "push rax\n"
                  "push rbx\n"
                  "ret\n"
        );
        out.print("BUILTIN_ftoi:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "cvttsd2si rax, [rbp-8]\n"
                  "push rax\n"
                  "pop rax\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "push rax\n"
                  "pop rax\n"
                  "pop rbx\n"
                  "push rax\n"
                  "push rbx\n"
                  "ret\n"
        );
        out.print("BUILTIN_itof:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "cvtsi2sd xmm0, [rbp-8]\n"
                  "movq rax, xmm0\n"
                  "push rax\n"
                  "pop rax\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "push rax\n"
                  "pop rax\n"
                  "pop rbx\n"
                  "push rax\n"
                  "push rbx\n"
                  "ret\n"
        );
        out.print("BUILTIN_sqrt:\n"
                  "mov rbp, rsp\n"
                  "add rbp, 8\n"
                  "movsd xmm0, [rbp-8]\n"
                  "sqrtsd xmm0, xmm0\n"
                  "movq rax, xmm0\n"
                  "push rax\n"
                  "pop rax\n"
                  "mov rsp, rbp\n"
                  "pop rbp\n"
                  "push rax\n"
                  "pop rax\n"
                  "pop rbx\n"
                  "push rax\n"
                  "push rbx\n"
                  "ret\n"
        );
        // out.print("putss:\n  mov rdx, rsi\n  mov rsi, rdi\n  mov eax, 1\n  mov edi, 1\n  syscall\n  ret\n");
        out.print("putcs:\n  mov rsi, rdi\n  mov rdx, -1\n.putcs_loop:\n  cmp BYTE [rsi+rdx+1], 0\n  lea rdx, [rdx+1]\n  jne .putcs_loop\n  mov eax, 1\n  mov edi, 1\n  syscall\n  ret\n");
        // out.print("putc:\n  push rdi\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea rsi, [rsp]\n  mov rdx, 1\n  syscall\n  pop rax\n  ret\n");
        out.print("puti:\n  mov rcx, rdi\n  sub rsp, 40\n  mov r10, rdi\n  mov r11, -3689348814741910323\n  neg rcx\n  cmovs rcx, rdi\n  mov edi, 22-1\n.puti_loop:\n  mov rax, rcx\n  movsx r9, dil\n  mul r11\n  movsx r8, r9b\n  lea edi, [r9-1]\n  mov rsi, r9\n  shr rdx, 3\n  lea rax, [rdx+rdx*4]\n  add rax, rax\n  sub rcx, rax\n  add ecx, 48\n  mov BYTE [rsp+r8], cl\n  mov rcx, rdx\n  test rdx, rdx\n  jne .puti_loop\n  test r10, r10\n  jns .puti_done\n  movsx rax, dil\n  movsx r9d, dil\n  movsx rsi, dil\n  mov BYTE [rsp+rax], 45\n.puti_done:\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea r10, [rsp+rsi]\n  mov rsi, r10\n  mov rdx, 22\n  sub edx, r9d\n  syscall\n  add rsp, 40\n  ret\n");
        out.print("putf:\n  push rbp\n  cvttsd2si rbp, xmm0\n  push rbx\n  mov rbx, rdi\n  sub rsp, 72\n  mov rdi, rbp\n  movsd QWORD [rsp+8], xmm0\n  call puti\n  movsd xmm0, QWORD [rsp+8]\n  pxor xmm1, xmm1\n  comisd xmm1, xmm0\n  jbe .putf_pos\n  mov r8, 0x8000000000000000\n  mov QWORD [rsp], r8\n  movsd xmm1, QWORD [rsp]\n  xorpd xmm0, xmm1\n  cvttsd2si rbp, xmm0\n.putf_pos:\n  pxor xmm1, xmm1\n  mov BYTE [rsp+16], 46\n  cvtsi2sd xmm1, rbp\n  subsd xmm0, xmm1\n  test rbx, rbx\n  je .putf_single\n  lea rax, [rsp+17]\n  mov r8, 0x4024000000000000\n  mov QWORD [rsp], r8\n  movsd xmm2, QWORD [rsp]\n  lea rdi, [rax+rbx]\n.putf_loop:\n  mulsd xmm0, xmm2\n  pxor xmm1, xmm1\n  add rax, 1\n  cvttsd2si rdx, xmm0\n  cvtsi2sd xmm1, rdx\n  lea ecx, [rdx+48]\n  mov BYTE [rax-1], cl\n  subsd xmm0, xmm1\n  cmp rdi, rax\n  jne .putf_loop\n  lea rdx, [rbx+1]\n.putf_done:\n  lea rsi, [rsp+16]\n  mov rax, 1\n  mov rdi, 1\n  syscall\n  add rsp, 72\n  pop rbx\n  pop rbp\n  ret\n.putf_single:\n  mov edx, 1\n  jmp .putf_done\n");
        // out.print("memcp:\n  test rdx, rdx\n  je .memcp_done\n  mov eax, 0\n.memcp_loop:\n  movzx ecx, BYTE [rsi+rax]\n  mov BYTE [rdi+rax], cl\n  add rax, 1\n  cmp rdx, rax\n  jne .memcp_loop\n.memcp_done:\n  ret\n");


        size_t ip = 0;
        for (const auto& proc : procedures) {
            out.print("trash_{}:\n", proc.procName);
            const auto& instructions = proc.instructions;
            for (size_t i = 0; i < instructions.size(); ++i, ++ip) {
                out.print("INS_{}:\n", ip);
                Instruction ins = instructions[i];

                // Hack because the stack actually grows down
                // Otherwise, the emulator would have to change
                // FIXME: This breaks compatability with C arrays
                if (ins.op.op_kind == ASTKind::ADD_BINARYOP_EXPR) {
                    if (i + 1 < instructions.size()) {
                        Instruction::Opcode nextKind = instructions[i + 1].opcode;
                        if (nextKind == Instruction::Opcode::DEREF ||
                            nextKind == Instruction::Opcode::STORE)
                        {
                            ins.op.op_kind = ASTKind::SUB_BINARYOP_EXPR;
                        }
                    }
                }

                switch (ins.opcode) {
                    case Instruction::Opcode::PUSH: {
#if DBG_INS
                        if (ins.lit.kind == TypeKind::I64) out.print("; PUSH {}\n", ins.lit.i64);
                        else if (ins.lit.kind == TypeKind::F64) out.print("; PUSH {}\n", ins.lit.f64);
                        else if (ins.lit.kind == TypeKind::U8) out.print("; PUSH {}\n", (char)ins.lit.u8);
                        else if (ins.lit.kind == TypeKind::STR) out.print("; PUSH \"{}\"\n", std::string_view{ins.lit.str.buf, ins.lit.str.sz});
                        else assert(0);
#endif
                        if (ins.lit.kind == TypeKind::I64) { out.print("push {}\n", ins.lit.i64); }
                        else if (ins.lit.kind == TypeKind::F64) {
                            out.print("push QWORD [REL FLOAT_{}]\n", floatLiteralPool.size());
                            floatLiteralPool.push_back(ins.lit.f64);
                        }
                        else if (ins.lit.kind == TypeKind::U8) { out.print("push {}\n", ins.lit.u8); }
                        else if (ins.lit.kind == TypeKind::STR) {
                            int64_t poolIdx = stringLiteralPool.size();
                            stringLiteralPool.emplace_back(UnescapeString(ins.lit.str.buf, ins.lit.str.sz));
                            out.print("lea rax, [REL STRING_{}]\n", poolIdx);
                            out.print("push rax\n"); // TOP = x
                        }
                        else assert(0);
                    } break;

                    case Instruction::Opcode::LOAD_FAST: {
#if DBG_INS
                        out.print("; LOAD_FAST {} ({})\n", ins.access.varAddr, ins.access.accessSize);
#endif
                        out.print("xor eax, eax\n");
                        out.print("mov {}, {} [rbp-{}-8]\n", // rax = *x
                                  ins.access.accessSize == 8 ? "rax" :
                                  ins.access.accessSize == 4 ? "eax" :
                                  ins.access.accessSize == 2 ? "ax" : "al",
                                  ins.access.accessSize == 8 ? "QWORD" :
                                  ins.access.accessSize == 4 ? "DWORD" :
                                  ins.access.accessSize == 2 ? "WORD" : "BYTE",
                                  ins.access.varAddr*8);
                        out.print("push rax\n"); // TOP = *x
                    } break;

                    case Instruction::Opcode::STORE: {
#if DBG_INS
                        out.print("; STORE ({})\n", ins.access.accessSize);
#endif
                        out.print("pop rax\n"); // TOP
                        out.print("pop rbx\n"); // TOP1
                        out.print("mov {} [rax-8], {}\n", // *TOP = TOP1
                                  ins.access.accessSize == 8 ? "QWORD" :
                                  ins.access.accessSize == 4 ? "DWORD" :
                                  ins.access.accessSize == 2 ? "WORD" : "BYTE",
                                  ins.access.accessSize == 8 ? "rbx" :
                                  ins.access.accessSize == 4 ? "ebx" :
                                  ins.access.accessSize == 2 ? "bx" : "bl");
                    } break;

                    case Instruction::Opcode::STORE_FAST: {
#if DBG_INS
                        out.print("; STORE_FAST {} ({})\n", ins.access.varAddr, ins.access.accessSize);
#endif
                        out.print("pop rax\n"); // TOP
                        out.print("mov {} [rbp-{}-8], {}\n", // *x = TOP
                                  ins.access.accessSize == 8 ? "QWORD" :
                                  ins.access.accessSize == 4 ? "DWORD" :
                                  ins.access.accessSize == 2 ? "WORD" : "BYTE",
                                  ins.access.varAddr*8,
                                  ins.access.accessSize == 8 ? "rax" :
                                  ins.access.accessSize == 4 ? "eax" :
                                  ins.access.accessSize == 2 ? "ax" : "al");
                    } break;

                    case Instruction::Opcode::ALLOCA: {
#if DBG_INS
                        out.print("; ALLOCA {}\n", ins.access.varAddr);
#endif
                        out.print("pop rax\n"); // TOP
                        out.print("mov QWORD [rbp-{}*8-8], rsp\n", // *x = addr
                                  ins.access.varAddr);
                        out.print("sub rsp, rax\n"); // sp = alloca(TOP)
                        out.print("sub rsp, 7\n");
                        out.print("mov rax, 0xFFFFFFFFFFFFFFF8\n");
                        out.print("and rsp, rax\n");
                    } break;

                    case Instruction::Opcode::DEREF: {
#if DBG_INS
                        out.print("; DEREF ({})\n", ins.access.accessSize);
#endif
                        out.print("pop rax\n"); // TOP
                        out.print("xor ebx, ebx\n");
                        out.print("mov {}, {} [rax-8]\n", // TOP = *TOP1
                                  ins.access.accessSize == 8 ? "rbx" :
                                  ins.access.accessSize == 4 ? "ebx" :
                                  ins.access.accessSize == 2 ? "bx" : "bl",
                                  ins.access.accessSize == 8 ? "QWORD" :
                                  ins.access.accessSize == 4 ? "DWORD" :
                                  ins.access.accessSize == 2 ? "WORD" : "BYTE");
                        out.print("push rbx\n");
                    } break;

                    case Instruction::Opcode::UNARY_OP: {
#if DBG_INS
                        if (ins.op.op_kind == ASTKind::NEG_UNARYOP_EXPR) out.print("; UNOP (NEG {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::NOT_UNARYOP_EXPR) out.print("; UNOP (NOT {})\n", TypeKindName(ins.op.kind));
                        else assert(0);
#endif
                        if (ins.op.kind == TypeKind::I64 || ins.op.kind == TypeKind::U8) {
                            out.print("pop rax\n");
                            if (ins.op.op_kind == ASTKind::NEG_UNARYOP_EXPR) {
                                out.print("neg rax\n");
                            }
                            else if (ins.op.op_kind == ASTKind::NOT_UNARYOP_EXPR) {
                                out.print("cmp rax, 0\n");
                                out.print("sete al\n");
                                out.print("movzx eax, al\n"); // Is this necessary?
                            }
                            else assert(0);
                            out.print("push rax\n");
                        }
                        else if (ins.op.kind == TypeKind::F64) {
                            if (ins.op.op_kind == ASTKind::NEG_UNARYOP_EXPR) {
                                out.print("movsd xmm0, [rsp]\n");
                                out.print("movsd xmm1, QWORD [REL DOUBLE_FLOAT_XOR]\n");
                                out.print("xorpd xmm0, xmm1\n");
                                out.print("movsd [rsp], xmm0\n");
                            }
                            else assert(0);
                        }
                        else assert(0);
                    } break;

                    case Instruction::Opcode::BINARY_OP: {
#if DBG_INS
                        if (ins.op.op_kind == ASTKind::EQ_BINARYOP_EXPR) out.print("; BINOP (EQ {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::NE_BINARYOP_EXPR) out.print("; BINOP (NE {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::GE_BINARYOP_EXPR) out.print("; BINOP (GE {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::GT_BINARYOP_EXPR) out.print("; BINOP (GT {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::LE_BINARYOP_EXPR) out.print("; BINOP (LE {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::LT_BINARYOP_EXPR) out.print("; BINOP (LT {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::AND_BINARYOP_EXPR) out.print("; BINOP (AND {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::OR_BINARYOP_EXPR) out.print("; BINOP (OR {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::ADD_BINARYOP_EXPR) out.print("; BINOP (ADD {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::SUB_BINARYOP_EXPR) out.print("; BINOP (SUB {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::MUL_BINARYOP_EXPR) out.print("; BINOP (MUL {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::DIV_BINARYOP_EXPR) out.print("; BINOP (DIV {})\n", TypeKindName(ins.op.kind));
                        else if (ins.op.op_kind == ASTKind::MOD_BINARYOP_EXPR) out.print("; BINOP (MOD {})\n", TypeKindName(ins.op.kind));
                        else assert(0);
#endif
                        if (ins.op.kind == TypeKind::I64 || ins.op.kind == TypeKind::U8) {
                            out.print("pop rbx\n"); // TOP
                            out.print("pop rax\n"); // TOP1

                            switch (ins.op.op_kind) {
                                case ASTKind::ADD_BINARYOP_EXPR: { out.print("add rax, rbx\n"); } break;
                                case ASTKind::SUB_BINARYOP_EXPR: { out.print("sub rax, rbx\n"); } break;
                                case ASTKind::MUL_BINARYOP_EXPR: { out.print("imul rax, rbx\n"); } break;
                                case ASTKind::DIV_BINARYOP_EXPR:
                                case ASTKind::MOD_BINARYOP_EXPR: {
                                    out.print("cqo\n");
                                    out.print("idiv rbx\n");
                                    if (ins.op.op_kind == ASTKind::MOD_BINARYOP_EXPR) {
                                        out.print("mov rax, rdx\n");
                                    }
                                } break;
                                case ASTKind::AND_BINARYOP_EXPR: {
                                    out.print("cmp rax, 0\n");
                                    out.print("setne al\n");
                                    out.print("cmp rbx, 0\n");
                                    out.print("setne bl\n");
                                    out.print("and rax, rbx\n");
                                } break;
                                case ASTKind::OR_BINARYOP_EXPR: {
                                    out.print("or rax, rbx\n");
                                    out.print("setne al\n");
                                    out.print("movzx eax, al\n");
                                } break;
                                case ASTKind::EQ_BINARYOP_EXPR:
                                case ASTKind::NE_BINARYOP_EXPR:
                                case ASTKind::GE_BINARYOP_EXPR:
                                case ASTKind::GT_BINARYOP_EXPR:
                                case ASTKind::LE_BINARYOP_EXPR:
                                case ASTKind::LT_BINARYOP_EXPR: {
                                    out.print("cmp rax, rbx\n");
                                    switch (ins.op.op_kind) {
                                        case ASTKind::EQ_BINARYOP_EXPR: out.print("sete al\n"); break;
                                        case ASTKind::NE_BINARYOP_EXPR: out.print("setne al\n"); break;
                                        case ASTKind::GE_BINARYOP_EXPR: out.print("setge al\n"); break;
                                        case ASTKind::GT_BINARYOP_EXPR: out.print("setg al\n"); break;
                                        case ASTKind::LE_BINARYOP_EXPR: out.print("setle al\n"); break;
                                        case ASTKind::LT_BINARYOP_EXPR: out.print("setl al\n"); break;
                                        default: assert(0);
                                    }
                                    out.print("movzx eax, al\n");
                                } break;

                                default: assert(0);
                            }

                            out.print("push rax\n");
                        }
                        else if (ins.op.kind == TypeKind::F64) {
                            switch (ins.op.op_kind) {
                                case ASTKind::ADD_BINARYOP_EXPR: {
                                    out.print("movsd xmm1, [rsp]\n");   // TOP
                                    out.print("movsd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("addsd xmm0, xmm1\n");
                                    out.print("add rsp, 8\n");
                                    out.print("movq [rsp], xmm0\n");
                                } break;
                                case ASTKind::SUB_BINARYOP_EXPR: {
                                    out.print("movsd xmm1, [rsp]\n");   // TOP
                                    out.print("movsd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("subsd xmm0, xmm1\n");
                                    out.print("add rsp, 8\n");
                                    out.print("movq [rsp], xmm0\n");
                                } break;
                                case ASTKind::MUL_BINARYOP_EXPR: {
                                    out.print("movsd xmm1, [rsp]\n");   // TOP
                                    out.print("movsd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("mulsd xmm0, xmm1\n");
                                    out.print("add rsp, 8\n");
                                    out.print("movq [rsp], xmm0\n");
                                } break;
                                case ASTKind::DIV_BINARYOP_EXPR: {
                                    out.print("movsd xmm1, [rsp]\n");   // TOP
                                    out.print("movsd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("divsd xmm0, xmm1\n");
                                    out.print("add rsp, 8\n");
                                    out.print("movq [rsp], xmm0\n");
                                } break;
                                case ASTKind::MOD_BINARYOP_EXPR: {
                                    out.print("movsd xmm1, [rsp]\n");   // TOP
                                    out.print("movsd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("fld QWORD [rsp+8]\n"
                                              "fld QWORD [rsp]\n"
                                              "fxch st1\n"
                                              ".fmod_loop:\n"
                                              "fprem\n"
                                              "fnstsw  ax\n"
                                              "sahf\n"
                                              "jp .fmod_loop\n"
                                              "fstp st1\n"
                                              "fstp QWORD [rsp-8]\n"
                                              "movsd xmm0, QWORD [rsp-8]\n"
                                              );
                                    out.print("add rsp, 8\n");
                                    out.print("movq [rsp], xmm0\n");
                                } break;
                                case ASTKind::AND_BINARYOP_EXPR:
                                case ASTKind::OR_BINARYOP_EXPR: {
                                    out.print("xorpd xmm0, xmm0\n");
                                    out.print("ucomisd xmm0, [rsp]\n"); // TOP
                                    out.print("setne al\n");
                                    out.print("xorpd xmm0, xmm0\n"); // Necessary?
                                    out.print("ucomisd xmm0, [rsp+8]\n"); // TOP1
                                    out.print("setne cl\n");
                                    if (ins.op.op_kind == ASTKind::AND_BINARYOP_EXPR) {
                                        out.print("and cl, al\n");
                                    }
                                    else {
                                        out.print("or cl, al\n");
                                    }
                                    out.print("movzx eax, cl\n");
                                    out.print("add rsp, 16\n");
                                    out.print("push rax\n");
                                } break;
                                case ASTKind::EQ_BINARYOP_EXPR:
                                case ASTKind::NE_BINARYOP_EXPR:
                                case ASTKind::GE_BINARYOP_EXPR:
                                case ASTKind::GT_BINARYOP_EXPR:
                                case ASTKind::LE_BINARYOP_EXPR:
                                case ASTKind::LT_BINARYOP_EXPR: {
                                    if (ins.op.op_kind == ASTKind::LE_BINARYOP_EXPR ||
                                        ins.op.op_kind == ASTKind::LT_BINARYOP_EXPR)
                                    {
                                        // swap regs
                                        out.print("movsd xmm0, [rsp]\n");
                                        out.print("xor eax, eax\n");
                                        out.print("comisd xmm0, [rsp+8]\n");
                                    }
                                    else {
                                        out.print("movsd xmm0, [rsp+8]\n");
                                        out.print("xor eax, eax\n");
                                        out.print("comisd xmm0, [rsp]\n");
                                    }
                                    switch (ins.op.op_kind) {
                                        case ASTKind::EQ_BINARYOP_EXPR: out.print("sete al\n"); break;
                                        case ASTKind::NE_BINARYOP_EXPR: out.print("setne al\n"); break;
                                        case ASTKind::GE_BINARYOP_EXPR:
                                        case ASTKind::GT_BINARYOP_EXPR:
                                        case ASTKind::LE_BINARYOP_EXPR: out.print("setnb al\n"); break;
                                        case ASTKind::LT_BINARYOP_EXPR: out.print("seta al\n"); break;
                                        default: assert(0);
                                    }

                                    out.print("movzx eax, al\n");
                                    out.print("add rsp, 16\n");
                                    out.print("push rax\n");
                                } break;

                                default: assert(0);
                            }
                        }
                        else assert(0);
                    } break;

                    case Instruction::Opcode::CALL: {
                        std::string_view sv{ins.lit.str.buf, ins.lit.str.sz};
#if DBG_INS
                        out.print("; CALL EXTERN {}\n", sv);
#endif
                        out.print("jmp extern_{}\n", sv);
                    } break;

                    case Instruction::Opcode::JMP: {
                        if (IS_BUILTIN(ins.jmpAddr)) {
#if DBG_INS
                            out.print("; CALL {}\n", (int64_t) ins.jmpAddr);
#endif
                            if (ins.jmpAddr == BUILTIN_putf) out.print("jmp BUILTIN_putf\n");
                            else if (ins.jmpAddr == BUILTIN_puti) out.print("jmp BUILTIN_puti\n");
                            else if (ins.jmpAddr == BUILTIN_puts) out.print("jmp BUILTIN_puts\n");
                            else if (ins.jmpAddr == BUILTIN_itoc) out.print("jmp BUILTIN_itoc\n");
                            else if (ins.jmpAddr == BUILTIN_ctoi) out.print("jmp BUILTIN_ctoi\n");
                            else if (ins.jmpAddr == BUILTIN_itof) out.print("jmp BUILTIN_itof\n");
                            else if (ins.jmpAddr == BUILTIN_ftoi) out.print("jmp BUILTIN_ftoi\n");
                            else if (ins.jmpAddr == BUILTIN_sqrt) out.print("jmp BUILTIN_sqrt\n");
                            else {
                                fmt::print(stderr, "{}\n", (int64_t) ins.jmpAddr);
                                assert(0);
                            }
                        }
                        else {
#if DBG_INS
                            out.print("; JMP {}\n", ins.jmpAddr);
#endif
                            out.print("jmp INS_{}\n", ins.jmpAddr); // ip = x
                        }
                    } break;

                    case Instruction::Opcode::JMP_NZ: {
#if DBG_INS
                        out.print("; JMP_NZ {}\n", ins.jmpAddr);
#endif
                        out.print("pop rax\n");
                        out.print("cmp rax, 0\n");
                        out.print("jne INS_{}\n", ins.jmpAddr); // if TOP != 0: ip = x
                    } break;

                    case Instruction::Opcode::JMP_Z: {
#if DBG_INS
                        out.print("; JMP_Z {}\n", ins.jmpAddr);
#endif
                        out.print("pop rax\n");
                        out.print("cmp rax, 0\n");
                        out.print("je INS_{}\n", ins.jmpAddr); // if TOP == 0: ip = x
                    } break;

                    case Instruction::Opcode::SAVE: {
#if DBG_INS
                        out.print("; SAVE {}\n", ins.jmpAddr);
#endif
                        out.print("lea rax, [REL INS_{}]\n", ins.jmpAddr);
                        out.print("push rax\n");
                        out.print("push rbp\n");
                    } break;

                    case Instruction::Opcode::ENTER: {
#if DBG_INS
                        out.print("; ENTER {} {}\n", ins.frame.numParams, ins.frame.numLocals);
#endif
                        out.print("mov rbp, rsp\n");
                        out.print("add rbp, {}\n", ins.frame.numParams*8); // bp = sp - x
                        out.print("sub rsp, {}\n", ins.frame.numLocals*8); // sp += y
                    } break;

                    case Instruction::Opcode::RETURN_VOID: {
#if DBG_INS
                        out.print("; RETURN_VOID\n");
#endif
                        out.print("mov rsp, rbp\n"); // sp = bp
                        out.print("pop rbp\n"); // bp = TOP
                        out.print("ret\n"); // jmp TOP
                    } break;

                    case Instruction::Opcode::RETURN_VAL: {
#if DBG_INS
                        out.print("; RETURN_VAL\n");
#endif
                        out.print("pop rax\n"); // retval = TOP
                        out.print("mov rsp, rbp\n"); // sp = bp
                        out.print("pop rbp\n"); // bp = TOP
                        out.print("push rax\n"); // TOP = retval
                        // swap retval and return addr
                        out.print("pop rax\n");
                        out.print("pop rbx\n");
                        out.print("push rax\n");
                        out.print("push rbx\n");

                        out.print("ret\n"); // jmp TOP
                    } break;

                    default: break;
                }
            }

        }
        for (size_t i = 0, n = stringLiteralPool.size(); i < n; ++i) {
            out.print("STRING_{} db ", i);
            for (char c : stringLiteralPool[i])
            out.print("{},", (int)c);
            out.print("0\n");
            // out.print("STRING_LEN_{} equ $ - STRING_{} - 1\n", i, i);
        }
        for (size_t i = 0, n = floatLiteralPool.size(); i < n; ++i) {
            double f = floatLiteralPool[i];
            uint64_t x;
            memcpy(&x, &f, 8);
            out.print("FLOAT_{} dq {} ; {}\n", i, x, f);
        }
        out.print("DOUBLE_FLOAT_XOR:\n"
                  "dq 0x8000000000000000\n");
    }
    else if (target == Target::POSIX_C) {
        assert(0 && "Target POSIX_C is not implemented yet.");
    }
}
