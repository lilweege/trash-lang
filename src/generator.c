#include "generator.h"

#include <string.h>
#include <assert.h>

size_t rspOffset = 40;

void generateProgram(const char* filename, AST* program) {
    size_t fnLen = strlen(filename);
    char outputFilename[fnLen+5];
    memcpy(outputFilename, filename, fnLen);
    memcpy(outputFilename+fnLen, ".asm", 5);
    FileWriter asmWriter = fwCreate(outputFilename);
    fwWriteChunkOrCrash(&asmWriter, "; compiler-generated statically linked Linux x86-64 NASM file\n");
    fwWriteChunkOrCrash(&asmWriter, "; compile and link with the following command:\n");
    fwWriteChunkOrCrash(&asmWriter, "; `nasm -felf64 -o %s.o %s.asm && ld -o %s %s.o`\n", filename, filename, filename, filename);
    fwWriteChunkOrCrash(&asmWriter, "BITS 64\n");
    fwWriteChunkOrCrash(&asmWriter, "global _start\n");
    fwWriteChunkOrCrash(&asmWriter, "section .data\n");
    fwWriteChunkOrCrash(&asmWriter, "section .text\n");
    fwWriteChunkOrCrash(&asmWriter, "puti:\n  mov rcx, rdi\n  sub rsp, 40\n  mov r10, rdi\n  mov r11, -3689348814741910323\n  neg rcx\n  cmovs rcx, rdi\n  mov edi, 22-1\n.puti_loop:\n  mov rax, rcx\n  movsx r9, dil\n  mul r11\n  movsx r8, r9b\n  lea edi, [r9-1]\n  mov rsi, r9\n  shr rdx, 3\n  lea rax, [rdx+rdx*4]\n  add rax, rax\n  sub rcx, rax\n  add ecx, 48\n  mov BYTE [rsp+r8], cl\n  mov rcx, rdx\n  test rdx, rdx\n  jne .puti_loop\n  test r10, r10\n  jns .puti_done\n  movsx rax, dil\n  movsx r9d, dil\n  movsx rsi, dil\n  mov BYTE [rsp+rax], 45\n.puti_done:\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea r10, [rsp+rsi]\n  mov rsi, r10\n  mov rdx, 22\n  sub edx, r9d\n  syscall\n  add rsp, 40\n  ret\n");
    fwWriteChunkOrCrash(&asmWriter, "putc:\n  push rdi\n  mov rax, 1 ; sys_write\n  mov rdi, 1 ; stdout\n  lea rsi, [rsp]\n  mov rdx, 1\n  syscall\n  pop rax\n  ret\n");
    fwWriteChunkOrCrash(&asmWriter, "_start:\n");
    // ...

    HashMap symbolTable = hmNew(256);
    generateStatements(program->right, &symbolTable, &asmWriter);
    hmFree(symbolTable);

    fwWriteChunkOrCrash(&asmWriter, "  mov rax, 60\n");
    fwWriteChunkOrCrash(&asmWriter, "  xor rdi, rdi\n");
    fwWriteChunkOrCrash(&asmWriter, "  syscall\n");
    fwWriteChunkOrCrash(&asmWriter, "section .bss\n");
    // ...

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
        
        fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rsp-%d], 0\n", conditionResult.offset);
        fwWriteChunkOrCrash(asmWriter, "  je .fi%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);

        AST* ifBody = conditional->right;
        if (ifBody->kind == NODE_BLOCK) {
            AST* first = ifBody->right;
            generateStatements(first, symbolTable, asmWriter);
        }
        else {
            generateStatement(ifBody, symbolTable, asmWriter);
        }

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
            if (elseBody->kind == NODE_BLOCK) {
                AST* first = elseBody->right;
                generateStatements(first, symbolTable, asmWriter);
            }
            else {
                generateStatement(elseBody, symbolTable, asmWriter);
            }
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
        
        fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rsp-%d], 0\n", conditionResult.offset);
        fwWriteChunkOrCrash(asmWriter, "  je .done%d_%d\n",
                    condition->token.pos.line,
                    condition->token.pos.col);
    
        AST* body = conditional->right;
        if (body->kind == NODE_BLOCK) {
            AST* first = body->right;
            generateStatements(first, symbolTable, asmWriter);
        }
        else {
            generateStatement(body, symbolTable, asmWriter);
        }

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
        rspOffset += 8;
        Token id = statement->token;
        
        Symbol newVar = (Symbol) {
            .id = id.text,
            .val = {
                .type = {
                    .kind = TYPE_I64,
                    .size = 0
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
        size_t offset = var->val.offset;
        fwWriteChunkOrCrash(asmWriter, "  ; "SV_FMT" has offset %d\n", SV_ARG(id.text), offset);
        size_t stackTop = rspOffset;
        Value rv = generateExpression(rval, symbolTable, asmWriter);
        rspOffset = stackTop;
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_ASSIGN\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", rv.offset);
        fwWriteChunkOrCrash(asmWriter, "  mov [rsp-%d], rax\n", offset);
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

    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTI\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, [rsp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  call puti\n");
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTC\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, [rsp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  call putc\n");
    }
    else {
        assert(0 && "Unimplemented");
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
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_INTEGER\n");
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rsp-%d], "SV_FMT"\n", rspOffset, SV_ARG(expression->token.text));
        Value lit = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
        return lit;
    }
    else if (expression->kind == NODE_CHAR) {
        rspOffset += 8;
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
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rsp-%d], %d\n", rspOffset, ch);
        Value lit = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
        return lit;
    }
    else if (expression->kind == NODE_CALL) {
        return generateCall(expression, symbolTable, asmWriter);
    }
    else if (expression->kind == NODE_LVALUE) {
        Token id = expression->token;
        Symbol* var = hmGet(symbolTable, id.text);
        return var->val;
    }

    // operators
    else if (expression->right == NULL) {
        // unary operator
        Value a = generateExpression(expression->left, symbolTable, asmWriter);
        switch (expression->kind) {
            case NODE_NEG: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  neg rax\n");
            } break;
            case NODE_NOT: {
                fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rsp-%d], 0\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  sete al\n");
                fwWriteChunkOrCrash(asmWriter, "  movzx eax, al\n");
            } break;
            default: assert(0);
        }
    }
    else {
        // binary operator
        Value a = generateExpression(expression->left, symbolTable, asmWriter);
        Value b = generateExpression(expression->right, symbolTable, asmWriter);
        switch (expression->kind) {
            case NODE_ADD: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  add rax, [rsp-%d]\n", b.offset);
            } break;
            case NODE_SUB: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  sub rax, [rsp-%d]\n", b.offset);
            } break;
            case NODE_MUL: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  imul rax, [rsp-%d]\n", b.offset);
            } break;
            case NODE_DIV:
            case NODE_MOD: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  cqo\n");
                fwWriteChunkOrCrash(asmWriter, "  idiv QWORD [rsp-%d]\n", b.offset);
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
                fwWriteChunkOrCrash(asmWriter, "  mov rax, [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  cmp rax, [rsp-%d]\n", b.offset);
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
                fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rsp-%d], 0\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  setne al\n");
                fwWriteChunkOrCrash(asmWriter, "  xor edi, edi\n");
                fwWriteChunkOrCrash(asmWriter, "  cmp QWORD [rsp-%d], 0\n", b.offset);
                fwWriteChunkOrCrash(asmWriter, "  setne dil\n");
                fwWriteChunkOrCrash(asmWriter, "  and rax, rdi\n");
            } break;
            case NODE_OR: {
                fwWriteChunkOrCrash(asmWriter, "  mov rax, QWORD [rsp-%d]\n", a.offset);
                fwWriteChunkOrCrash(asmWriter, "  xor edi, edi\n");
                fwWriteChunkOrCrash(asmWriter, "  or rax, QWORD [rsp-%d]\n", b.offset);
                fwWriteChunkOrCrash(asmWriter, "  setne al\n");
                fwWriteChunkOrCrash(asmWriter, "  movzx eax, al\n");
            } break;

            default: assert(0);
        }
    }

    rspOffset += 8;
    fwWriteChunkOrCrash(asmWriter, "  mov [rsp-%d], rax\n", rspOffset);
    return (Value) {
        .type = {
            .kind = TYPE_I64,
            .size = 0
        },
        .offset = rspOffset
    };
}
