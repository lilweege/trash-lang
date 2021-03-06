#include "generator.h"

#include <string.h>
#include <assert.h>

size_t rspOffset = 128;

StringView staticStrings[1024];
size_t numStrings = 0;

void generateProgram(const char* outputFilename, AST* program, Target target) {
    FileWriter asmWriter = fwCreate(outputFilename);

    // TODO: windows stuff
    // the primary differences between windows and linux are the calling conventions and external calls
    // linux simply has `syscall` but windows has to pull in external symbols and link with kernel32.lib
    // https://www.cs.uaf.edu/2017/fall/cs301/reference/x86_64.html
    // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170

    // necessary preamble
    if (target == TARGET_LINUX) {
        fwWriteChunkOrCrash(&asmWriter, "; compiler-generated Linux x86-64 NASM file\n");
    }
    else if (target == TARGET_WINDOWS) {
        fwWriteChunkOrCrash(&asmWriter, "; compiler-generated Windows x86-64 NASM file\n");
        fwWriteChunkOrCrash(&asmWriter, "extern ExitProcess\n");
        fwWriteChunkOrCrash(&asmWriter, "extern GetStdHandle\n");
        fwWriteChunkOrCrash(&asmWriter, "extern WriteFile\n");
        fwWriteChunkOrCrash(&asmWriter, "extern __chkstk\n");
    }

    fwWriteChunkOrCrash(&asmWriter, "BITS 64\n");
    fwWriteChunkOrCrash(&asmWriter, "global _start\n");
    fwWriteChunkOrCrash(&asmWriter, "section .text\n");
    if (target == TARGET_WINDOWS) {
        // TODO: implement all the functions
        fwWriteChunkOrCrash(&asmWriter, "putss:\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putcs:\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putc:\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "puti:\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putf:\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "memcp:\n  ret\n");
    }
    else if (target == TARGET_LINUX) {
        fwWriteChunkOrCrash(&asmWriter, "putss:\n  mov rdx, rsi\n  mov rsi, rdi\n  mov eax, 1\n  mov edi, 1\n  syscall\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putcs:\n  mov rsi, rdi\n  mov rdx, -1\n.putcs_loop:\n  cmp BYTE [rsi+rdx+1], 0\n  lea rdx, [rdx+1]\n  jne .putcs_loop\n  mov eax, 1\n  mov edi, 1\n  syscall\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putc:\n  push rdi\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea rsi, [rsp]\n  mov rdx, 1\n  syscall\n  pop rax\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "puti:\n  mov rcx, rdi\n  sub rsp, 40\n  mov r10, rdi\n  mov r11, -3689348814741910323\n  neg rcx\n  cmovs rcx, rdi\n  mov edi, 22-1\n.puti_loop:\n  mov rax, rcx\n  movsx r9, dil\n  mul r11\n  movsx r8, r9b\n  lea edi, [r9-1]\n  mov rsi, r9\n  shr rdx, 3\n  lea rax, [rdx+rdx*4]\n  add rax, rax\n  sub rcx, rax\n  add ecx, 48\n  mov BYTE [rsp+r8], cl\n  mov rcx, rdx\n  test rdx, rdx\n  jne .puti_loop\n  test r10, r10\n  jns .puti_done\n  movsx rax, dil\n  movsx r9d, dil\n  movsx rsi, dil\n  mov BYTE [rsp+rax], 45\n.puti_done:\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea r10, [rsp+rsi]\n  mov rsi, r10\n  mov rdx, 22\n  sub edx, r9d\n  syscall\n  add rsp, 40\n  ret\n");
        fwWriteChunkOrCrash(&asmWriter, "putf:\n  push rbp\n  cvttsd2si rbp, xmm0\n  push rbx\n  mov rbx, rdi\n  sub rsp, 72\n  mov rdi, rbp\n  movsd QWORD [rsp+8], xmm0\n  call puti\n  movsd xmm0, QWORD [rsp+8]\n  pxor xmm1, xmm1\n  comisd xmm1, xmm0\n  jbe .putf_pos\n  mov r8, 0x8000000000000000\n  mov QWORD [rsp], r8\n  movsd xmm1, QWORD [rsp]\n  xorpd xmm0, xmm1\n  cvttsd2si rbp, xmm0\n.putf_pos:\n  pxor xmm1, xmm1\n  mov BYTE [rsp+16], 46\n  cvtsi2sd xmm1, rbp\n  subsd xmm0, xmm1\n  test rbx, rbx\n  je .putf_single\n  lea rax, [rsp+17]\n  mov r8, 0x4024000000000000\n  mov QWORD [rsp], r8\n  movsd xmm2, QWORD [rsp]\n  lea rdi, [rax+rbx]\n.putf_loop:\n  mulsd xmm0, xmm2\n  pxor xmm1, xmm1\n  add rax, 1\n  cvttsd2si rdx, xmm0\n  cvtsi2sd xmm1, rdx\n  lea ecx, [rdx+48]\n  mov BYTE [rax-1], cl\n  subsd xmm0, xmm1\n  cmp rdi, rax\n  jne .putf_loop\n  lea rdx, [rbx+1]\n.putf_done:\n  lea rsi, [rsp+16]\n  mov rax, 1\n  mov rdi, 1\n  syscall\n  add rsp, 72\n  pop rbx\n  pop rbp\n  ret\n.putf_single:\n  mov edx, 1\n  jmp .putf_done\n");
        fwWriteChunkOrCrash(&asmWriter, "memcp:\n  test rdx, rdx\n  je .memcp_done\n  mov eax, 0\n.memcp_loop:\n  movzx ecx, BYTE [rsi+rax]\n  mov BYTE [rdi+rax], cl\n  add rax, 1\n  cmp rdx, rax\n  jne .memcp_loop\n.memcp_done:\n  ret\n");
    }

    // entry point
    fwWriteChunkOrCrash(&asmWriter, "_start:\n");
    
    // prepare stack
    fwWriteChunkOrCrash(&asmWriter, "  push rbp\n");
    fwWriteChunkOrCrash(&asmWriter, "  mov rbp, rsp\n");
    // TODO: make this constant depend on the current stack frame
    // this will matter when implementing subroutines
    fwWriteChunkOrCrash(&asmWriter, "  mov rax, %d\n", 1<<23);
    if (target == TARGET_WINDOWS) {
        // this is important otherwise you will get memory access violation
	    fwWriteChunkOrCrash(&asmWriter, "  call __chkstk\n");
    }
    else if (target == TARGET_LINUX) {
    }
    // fwWriteChunkOrCrash(&asmWriter, "  sub rsp, rax\n");


    HashMap symbolTable = hmNew(256);
    generateStatements(program->right, &symbolTable, &asmWriter);
    hmFree(symbolTable);

    // clean up and exit
    // fwWriteChunkOrCrash(&asmWriter, "  leave\n");
    if (target == TARGET_WINDOWS) {
        fwWriteChunkOrCrash(&asmWriter, "  xor rcx, rcx\n");
        fwWriteChunkOrCrash(&asmWriter, "  call ExitProcess\n");
    }
    else if (target == TARGET_LINUX) {
        fwWriteChunkOrCrash(&asmWriter, "  mov rax, 60\n");
        fwWriteChunkOrCrash(&asmWriter, "  xor rdi, rdi\n");
        fwWriteChunkOrCrash(&asmWriter, "  syscall\n");
    }
    // data section (strings)
    fwWriteChunkOrCrash(&asmWriter, "section .data\n");

    for (size_t i = 0; i < numStrings; ++i) {
        fwWriteChunkOrCrash(&asmWriter, "  s%zu db ", i);
        StringView sv = staticStrings[i];
        for (size_t j = 0; j < sv.size; ++j) {
            char ch = sv.data[j];
            if (ch == '\\') {
                if (++j >= sv.size) {
                    assert(0);
                }
                char esc = sv.data[j];
                switch (esc) {
                    case '"': ch = '"'; break;
                    case 'n': ch = '\n'; break;
                    case 't': ch = '\t'; break;
                    case '\\': ch = '\\'; break;
                    default: assert(0);
                }
            }
            fwWriteChunkOrCrash(&asmWriter, "%d,", (int) ch);
        }
        fwWriteChunkOrCrash(&asmWriter, "0\n");
        fwWriteChunkOrCrash(&asmWriter, "  l%zu equ $ - s%zu - 1\n", i, i);
    }

    fwDestroy(asmWriter);
}

void generateStatements(AST* statement, HashMap* symbolTable, FileWriter* asmWriter) {
    if (statement->right == NULL) {
        return;
    }

    AST* toCheck = statement->left;
    AST* nextStatement = statement->right;
    if (toCheck->kind == NODE_WHILE || toCheck->kind == NODE_IF) {
        generateConditional(statement, symbolTable, asmWriter);
    }
    else {
        generateStatement(statement, symbolTable, asmWriter);
    }
    generateStatements(nextStatement, symbolTable, asmWriter);
}

void generateConditional(AST* statement, HashMap* symbolTable, FileWriter* asmWriter) {
    AST* conditional = statement->left;
    
    AST* condition = conditional->left;
    if (conditional->kind == NODE_IF) {
        AST* left = conditional->left;
        bool hasElse = left->kind == NODE_ELSE; // left is `else`
        if (hasElse) {
            condition = left->left;
        }

        fwWriteChunkOrCrash(asmWriter, "  ; NODE_IF\n");

        size_t stackTop = rspOffset;
        Value conditionResult = generateExpression(condition, symbolTable, asmWriter);
        rspOffset = stackTop;
        
        fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rbp-%d], 0\n", conditionResult.offset);
        fwWriteChunkOrCrash(asmWriter, "  je .fi%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);

        AST* ifBody = conditional->right;
        HashMap innerScope = hmCopy(*symbolTable);
        if (ifBody->kind == NODE_BLOCK) {
            AST* first = ifBody->right;
            generateStatements(first, &innerScope, asmWriter);
        }
        else {
            generateStatement(ifBody, &innerScope, asmWriter);
        }
        hmFree(innerScope);

        if (hasElse) {
            fwWriteChunkOrCrash(asmWriter, "  ; NODE_ELSE\n");
            fwWriteChunkOrCrash(asmWriter, "  jmp .done%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);
        }
        fwWriteChunkOrCrash(asmWriter, ".fi%d_%d:\n",
                    condition->token.pos.line,
                    condition->token.pos.col);

        if (hasElse) {
            AST* elseBody = left->right;
            innerScope = hmCopy(*symbolTable);
            if (elseBody->kind == NODE_BLOCK) {
                AST* first = elseBody->right;
                generateStatements(first, &innerScope, asmWriter);
            }
            else {
                generateStatement(elseBody, &innerScope, asmWriter);
            }
            hmFree(innerScope);
            fwWriteChunkOrCrash(asmWriter, ".done%d_%d:\n",
                        condition->token.pos.line,
                        condition->token.pos.col);
        }

    }
    else {
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_WHILE\n");
        fwWriteChunkOrCrash(asmWriter, ".loop%d_%d:\n",
                    condition->token.pos.line,
                    condition->token.pos.col);


        size_t stackTop = rspOffset;
        Value conditionResult = generateExpression(condition, symbolTable, asmWriter);
        rspOffset = stackTop;
        
        fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rbp-%d], 0\n", conditionResult.offset);
        fwWriteChunkOrCrash(asmWriter, "  je .done%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);
    
        AST* body = conditional->right;
        HashMap innerScope = hmCopy(*symbolTable);
        if (body->kind == NODE_BLOCK) {
            AST* first = body->right;
            generateStatements(first, &innerScope, asmWriter);
        }
        else {
            generateStatement(body, &innerScope, asmWriter);
        }
        hmFree(innerScope);

        fwWriteChunkOrCrash(asmWriter, "  jmp .loop%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);
        fwWriteChunkOrCrash(asmWriter, ".done%d_%d:\n",
                    condition->token.pos.line,
                    condition->token.pos.col);
    }
}

void generateStatement(AST* wrapper, HashMap* symbolTable, FileWriter* asmWriter) {
    AST* statement = wrapper->left;
    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        generateConditional(wrapper, symbolTable, asmWriter);
    }
    else if (statement->kind == NODE_DEFINITION) {
        Token id = statement->token;

        int64_t arrSize = 0;
        AST* typeNode = statement->left;
        AST* subscript = typeNode->left;
        bool hasSubscript = subscript != NULL;
        if (hasSubscript) {
            arrSize = svParseI64(subscript->token.text);
        }
        assert(arrSize >= 0);

        // TODO: clean this up
        Token typeName = typeNode->token;
        TypeKind assignTypeKind = svCmp(svFromCStr("u8"), typeName.text) == 0 ? TYPE_U8 :
                                svCmp(svFromCStr("i64"), typeName.text) == 0 ? TYPE_I64 : TYPE_F64;
        
        size_t offset = (size_t)(typeKindSize(assignTypeKind) * (arrSize == 0 ? 1 : arrSize));
        offset += 8 - ((rspOffset-1) & (8-1)) - 1;
        rspOffset += offset;
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_DEFINITION "SV_FMT" has offset %d\n", SV_ARG(id.text), rspOffset);

        Symbol newVar = (Symbol) {
            .id = id.text,
            .val = {
                .type = {
                    .kind = assignTypeKind,
                    .size = (size_t) arrSize
                },
                .offset = rspOffset
            }
        };
        hmPut(symbolTable, newVar);
    }
    else if (statement->kind == NODE_ASSIGN) {
        AST* lval = statement->left;
        AST* rval = statement->right;
        Token id = lval->token;
        Symbol* var = hmGet(symbolTable, id.text);
        size_t stackTop = rspOffset;

        AST* subscript = lval->left;
        bool hasSubscript = subscript != NULL;

        Value rv = generateExpression(rval, symbolTable, asmWriter);
        if (var->val.type.kind == TYPE_U8 &&
                var->val.type.size != 0 &&
                rv.type.kind == TYPE_STR) {
            // special case string assign
            fwWriteChunkOrCrash(asmWriter, "  ; NODE_ASSIGN string\n");
            fwWriteChunkOrCrash(asmWriter, "  lea rdi, [rbp-%d]\n", var->val.offset); // dst
            fwWriteChunkOrCrash(asmWriter, "  mov rsi, [rbp-%d+8]\n", rv.offset); // src
            fwWriteChunkOrCrash(asmWriter, "  mov rdx, [rbp-%d]\n", rv.offset); // cnt
            fwWriteChunkOrCrash(asmWriter, "  inc rdx\n"); // copy null byte
            fwWriteChunkOrCrash(asmWriter, "  call memcp\n");
        }
        else {
            // TODO: I think this is still very broken for chars
            if (hasSubscript) {
                fwWriteChunkOrCrash(asmWriter, "  ; NODE_ASSIGN subscript\n");
                Value indexResult = generateExpression(subscript, symbolTable, asmWriter);
                fwWriteChunkOrCrash(asmWriter, "  mov rcx, [rbp-%d]\n", indexResult.offset);
                // fwWriteChunkOrCrash(asmWriter, "  neg rcx\n");
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", rv.offset);
                if (var->val.type.kind == TYPE_U8) {
                    fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d+rcx], al\n", var->val.offset);
                }
                else {
                    fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d+rcx*8], rax\n", var->val.offset);
                }
            }
            else {
                fwWriteChunkOrCrash(asmWriter, "  ; NODE_ASSIGN\n");
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", rv.offset);
                if (var->val.type.kind == TYPE_U8) {
                    fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], al\n", var->val.offset);
                }
                else {
                    fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", var->val.offset);
                }
            }
        }

        rspOffset = stackTop;
    }
    else {
        size_t stackTop = rspOffset;
        generateExpression(statement, symbolTable, asmWriter);
        rspOffset = stackTop;
    }
}

Value generateCall(AST* call, HashMap* symbolTable, FileWriter* asmWriter) {
    AST* args = call->left;
    Value arguments[MAX_FUNC_ARGS];
    size_t numArgs = 0;

    for (AST* curArg = args;
         curArg->right != NULL;
         curArg = curArg->right)
    {
        AST* arg = curArg->left;
        size_t stackTop = rspOffset;
        arguments[numArgs++] = generateExpression(arg, symbolTable, asmWriter);
        rspOffset = stackTop;
    }

    if (svCmp(svFromCStr("puts"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTS\n");
        if (arguments[0].type.kind == TYPE_STR) {
            fwWriteChunkOrCrash(asmWriter, "  ; STR LIT\n");
            fwWriteChunkOrCrash(asmWriter, "  mov rdi, [rbp-%d+8]\n", arguments[0].offset); // addr
            fwWriteChunkOrCrash(asmWriter, "  mov rsi, [rbp-%d]\n", arguments[0].offset); // size
            fwWriteChunkOrCrash(asmWriter, "  call putss\n");
        }
        else {
            fwWriteChunkOrCrash(asmWriter, "  ; CHAR ARRAY\n");
            fwWriteChunkOrCrash(asmWriter, "  lea rdi, [rbp-%d]\n", arguments[0].offset);
            fwWriteChunkOrCrash(asmWriter, "  call putcs\n");
        }
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTC\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, [rbp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  call putc\n");
    }
    else if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTI\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, [rbp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  call puti\n");
    }
    else if (svCmp(svFromCStr("putf"), call->token.text) == 0) {
        const size_t precision = 8; // TODO: make this a (optional) parameter
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTF\n");
        fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, %zu\n", precision);
        fwWriteChunkOrCrash(asmWriter, "  call putf\n");
    }
    else if (svCmp(svFromCStr("itof"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; ITOF\n");
        fwWriteChunkOrCrash(asmWriter, "  cvtsi2sd xmm0, [rbp-%d]\n", arguments[0].offset); // nice instruction
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  movq [rbp-%d], xmm0\n", rspOffset);
        return (Value) {
            .type = {
                .kind = TYPE_F64,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else if (svCmp(svFromCStr("ftoi"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; FTOI\n");
        fwWriteChunkOrCrash(asmWriter, "  cvttsd2si rax, [rbp-%d]\n", arguments[0].offset); // nice instruction
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
        return (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else if (svCmp(svFromCStr("itoc"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; ITOC\n");
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  movzx eax, BYTE [rbp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], eax\n", rspOffset);
        return (Value) {
            .type = {
                .kind = TYPE_U8,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else if (svCmp(svFromCStr("ctoi"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CTOI\n");
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  movzx eax, BYTE [rbp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], eax\n", rspOffset);
        return (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else {
        printf(SV_FMT" UNIMPLEMENTED\n", SV_ARG(call->token.text));
        assert(0 );
    }

    return (Value) {
        .type = {
            .kind = TYPE_NONE,
            .size = 0
        },
        .offset = 0
    };
}

Value generateExpression(AST* expression, HashMap* symbolTable, FileWriter* asmWriter) {
    if (expression->kind == NODE_INTEGER) {
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_INTEGER\n");
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rbp-%d], "SV_FMT"\n", rspOffset, SV_ARG(expression->token.text));
        Value lit = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
        return lit;
    }
    else if (expression->kind == NODE_FLOAT) {
        double literal = svParseF64(expression->token.text);
        int64_t double_raw;
        memcpy(&double_raw, &literal, 8);
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_FLOAT\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rax, 0x%lX ; %.16f\n", double_raw, literal);
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);

        return (Value) {
            .type = {
                .kind = TYPE_F64,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else if (expression->kind == NODE_CHAR) {
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_CHAR\n");
        char ch = expression->token.text.data[0];
        if (expression->token.text.size > 1) {
            switch (expression->token.text.data[1]) {
                case '"': ch = '"'; break;
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case '\\': ch = '\\'; break;
                default: assert(0);
            }
        }
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov BYTE [rbp-%d], %d\n", rspOffset, ch);
        Value lit = (Value) {
            .type = {
                .kind = TYPE_U8,
                .size = 0
            },
            .offset = rspOffset
        };
        return lit;
    }
    else if (expression->kind == NODE_STRING) {
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_STRING\n");

        // push addr to stack
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rbp-%d], s%d\n", rspOffset, numStrings);
        // push size to stack
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rbp-%d], l%d\n", rspOffset, numStrings);
        
        // to access them
            // [rbp-%d+8]   -> addr
            // [rbp-%d] -> size
        
        staticStrings[numStrings++] = expression->token.text;
        return (Value) {
            .type = {
                .kind = TYPE_STR,
                .size = 0
            },
            .offset = rspOffset
        };
    }
    else if (expression->kind == NODE_CALL) {
        return generateCall(expression, symbolTable, asmWriter);
    }
    else if (expression->kind == NODE_LVALUE) {
        Token id = expression->token;
        Symbol* var = hmGet(symbolTable, id.text);

        AST* subscript = expression->left;
        bool hasSubscript = subscript != NULL;
        if (hasSubscript) {
            fwWriteChunkOrCrash(asmWriter, "  ; NODE_LVALUE subscript\n");
            Value indexResult = generateExpression(subscript, symbolTable, asmWriter);
            fwWriteChunkOrCrash(asmWriter, "  mov rcx, [rbp-%d]\n", indexResult.offset);
            // fwWriteChunkOrCrash(asmWriter, "  neg rcx\n");
            // INDEX IN RCX

            rspOffset += 8;
            if (var->val.type.kind == TYPE_U8) {
                fwWriteChunkOrCrash(asmWriter, "  mov al, [rbp-%d+rcx] \n", var->val.offset);
                fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], al\n", rspOffset);
            }
            else {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d+rcx*8] \n", var->val.offset);
                fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
            }
            return (Value) {
                .type = {
                    .kind = var->val.type.kind,
                    .size = 0
                },
                .offset = rspOffset
            };
        }
        return var->val;
    }

    // operators
    else if (expression->right == NULL) {
        // unary operator
        Value a = generateExpression(expression->left, symbolTable, asmWriter);
        if (a.type.kind == TYPE_I64) {
            switch (expression->kind) {
                case NODE_NEG: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER NEG\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  neg rax\n");
                } break;
                case NODE_NOT: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER BOOL NOT\n");
                    fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rbp-%d], 0\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  sete al\n");
                    fwWriteChunkOrCrash(asmWriter, "  movzx eax, al\n");
                } break;
                default: assert(0);
            }
            rspOffset += 8;
            fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
            return (Value) {
                .type = {
                    .kind = TYPE_I64,
                    .size = 0
                },
                .offset = rspOffset
            };
        }
        else if (a.type.kind == TYPE_F64) {
            switch (expression->kind) {
                case NODE_NEG: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT NEG\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  mov r8, 0x8000000000000000\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d-8], r8\n", rspOffset);
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm1, [rbp-%d-8]\n", rspOffset);
                    fwWriteChunkOrCrash(asmWriter, "  xorpd xmm0, xmm1\n");
                } break;
                case NODE_NOT: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT BOOL NOT\n");
                    fwWriteChunkOrCrash(asmWriter, "  xorpd xmm0, xmm0\n");
                    fwWriteChunkOrCrash(asmWriter, "  xor eax, eax\n");
                    fwWriteChunkOrCrash(asmWriter, "  ucomisd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  sete al\n");
                } break;
                default: assert(0);
            }
            TypeKind resultKind = unaryResultTypeKind(a.type.kind, expression->kind);
            rspOffset += 8;
            if (resultKind == TYPE_I64) {
                fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
            }
            else if (resultKind == TYPE_F64) {
                fwWriteChunkOrCrash(asmWriter, "  movsd [rbp-%d], xmm0\n", rspOffset);
            }
            else {
                assert(0 && "Invalid result type kind");
            }
            return (Value) {
                .type = {
                    .kind = resultKind,
                    .size = 0
                },
                .offset = rspOffset
            };
        }
        else {
            assert(0 && "unary operation unsupported type");
        }
    }
    else {
        // binary operator
        Value a = generateExpression(expression->left, symbolTable, asmWriter);
        Value b = generateExpression(expression->right, symbolTable, asmWriter);
        assert(a.type.kind == b.type.kind);
        if (a.type.kind == TYPE_I64) {
            switch (expression->kind) {
                case NODE_ADD: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER ADD\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  add rax, [rbp-%d]\n", b.offset);
                } break;
                case NODE_SUB: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER SUB\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  sub rax, [rbp-%d]\n", b.offset);
                } break;
                case NODE_MUL: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER MUL\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  imul rax, [rbp-%d]\n", b.offset);
                } break;
                case NODE_DIV:
                case NODE_MOD: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER DIV/MOD\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  cqo\n");
                    fwWriteChunkOrCrash(asmWriter, "  idiv QWORD [rbp-%d]\n", b.offset);
                    if (expression->kind == NODE_MOD) {
                        fwWriteChunkOrCrash(asmWriter, "  mov rax, rdx\n");
                    }
                } break;
                case NODE_NE:
                case NODE_EQ:
                case NODE_GT:
                case NODE_LT:
                case NODE_GE:
                case NODE_LE: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER CMP\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  cmp rax, [rbp-%d]\n", b.offset);
                    switch (expression->kind) {
                        case NODE_NE: fwWriteChunkOrCrash(asmWriter, "  setne al\n"); break;
                        case NODE_EQ: fwWriteChunkOrCrash(asmWriter, "  sete al\n"); break;
                        case NODE_GT: fwWriteChunkOrCrash(asmWriter, "  setg al\n"); break;
                        case NODE_LT: fwWriteChunkOrCrash(asmWriter, "  setl al\n"); break;
                        case NODE_GE: fwWriteChunkOrCrash(asmWriter, "  setge al\n"); break;
                        case NODE_LE: fwWriteChunkOrCrash(asmWriter, "  setle al\n"); break;
                        default: assert(0);
                    }
                    fwWriteChunkOrCrash(asmWriter, "  movzx eax, al\n");
                } break;
                case NODE_AND: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER BOOL AND\n");
                    fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rbp-%d], 0\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  setne al\n");
                    fwWriteChunkOrCrash(asmWriter, "  xor edi, edi\n");
                    fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rbp-%d], 0\n", b.offset);
                    fwWriteChunkOrCrash(asmWriter, "  setne dil\n");
                    fwWriteChunkOrCrash(asmWriter, "  and rax, rdi\n");
                } break;
                case NODE_OR: {
                    fwWriteChunkOrCrash(asmWriter, "  ; INTEGER BOOL OR\n");
                    fwWriteChunkOrCrash(asmWriter, "  mov rax, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  xor edi, edi\n");
                    fwWriteChunkOrCrash(asmWriter, "  or rax, [rbp-%d]\n", b.offset);
                    fwWriteChunkOrCrash(asmWriter, "  setne al\n");
                    fwWriteChunkOrCrash(asmWriter, "  movzx eax, al\n");
                } break;

                default: assert(0);
            }
            rspOffset += 8;
            fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
            return (Value) {
                .type = {
                    .kind = TYPE_I64,
                    .size = 0
                },
                .offset = rspOffset
            };
        }
        else if (a.type.kind == TYPE_F64) {
            switch (expression->kind) {
                case NODE_ADD: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT ADD\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  addsd xmm0, [rbp-%d]\n", b.offset);
                } break;
                case NODE_SUB: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT SUB\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  subsd xmm0, [rbp-%d]\n", b.offset);
                } break;
                case NODE_MUL: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT MUL\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  mulsd xmm0, [rbp-%d]\n", b.offset);
                } break;
                case NODE_DIV: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT DIV\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  divsd xmm0, [rbp-%d]\n", b.offset);
                } break;
                case NODE_NE:
                case NODE_EQ:
                case NODE_GT:
                case NODE_LT:
                case NODE_GE:
                case NODE_LE: {
                    if (expression->kind == NODE_LT ||
                        expression->kind == NODE_LE) {
                        // swap operands (this is inconsequential, 
                        // it could be done 100 other ways as well)
                        size_t t = a.offset;
                        a.offset = b.offset;
                        b.offset = t;
                    }

                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT CMP\n");
                    fwWriteChunkOrCrash(asmWriter, "  movsd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  xor eax, eax\n");
                    fwWriteChunkOrCrash(asmWriter, "  comisd xmm0, [rbp-%d]\n", b.offset);
                    switch (expression->kind) {
                        case NODE_NE: fwWriteChunkOrCrash(asmWriter, "  setne al\n"); break;
                        case NODE_EQ: fwWriteChunkOrCrash(asmWriter, "  sete al\n"); break;
                        case NODE_GT:
                        case NODE_LT: fwWriteChunkOrCrash(asmWriter, "  seta al\n"); break;
                        case NODE_GE:
                        case NODE_LE: fwWriteChunkOrCrash(asmWriter, "  setnb al\n"); break;
                        default: assert(0);
                    }
                } break;
                case NODE_AND:
                case NODE_OR: {
                    fwWriteChunkOrCrash(asmWriter, "  ; FLOAT BOOL AND/OR\n");
                    fwWriteChunkOrCrash(asmWriter, "  xorpd xmm0, xmm0\n");
                    fwWriteChunkOrCrash(asmWriter, "  ucomisd xmm0, [rbp-%d]\n", a.offset);
                    fwWriteChunkOrCrash(asmWriter, "  setne al\n");
                    fwWriteChunkOrCrash(asmWriter, "  ucomisd xmm0, [rbp-%d]\n", b.offset);
                    fwWriteChunkOrCrash(asmWriter, "  setne cl\n");
                    if (expression->kind == NODE_AND) {
                        fwWriteChunkOrCrash(asmWriter, "  and cl, al\n");
                    }
                    else {
                        fwWriteChunkOrCrash(asmWriter, "  or cl, al\n");
                    }
                    fwWriteChunkOrCrash(asmWriter, "  movzx eax, cl\n");
                } break;

                default: assert(0);
            }

            TypeKind resultKind = binaryResultTypeKind(a.type.kind, b.type.kind, expression->kind);
            rspOffset += 8;
            if (resultKind == TYPE_I64) {
                fwWriteChunkOrCrash(asmWriter, "  mov [rbp-%d], rax\n", rspOffset);
            }
            else if (resultKind == TYPE_F64) {
                fwWriteChunkOrCrash(asmWriter, "  movsd [rbp-%d], xmm0\n", rspOffset);
            }
            else {
                assert(0 && "Invalid result type kind");
            }

            return (Value) {
                .type = {
                    .kind = resultKind,
                    .size = 0
                },
                .offset = rspOffset
            };
        }
        else {
            assert(0 && " binary operation unsupported type");
        }
    }

    // unreachable
    assert(0 && "Unreachable");
    return (Value) {
        .type = {
            .kind = TYPE_I64,
            .size = 0
        },
        .offset = 0
    };
}
