#include "generator.h"

#include <string.h>
#include <assert.h>

size_t rspOffset = 0;

void generateProgram(const char* filename, AST* program) {
    size_t fnLen = strlen(filename);
    char outputFilename[fnLen+5];
    memcpy(outputFilename, filename, fnLen);
    memcpy(outputFilename+fnLen, ".asm", 5);
    FileWriter asmWriter = fwCreate(outputFilename);
    fwWriteChunkOrCrash(&asmWriter, "; compiler-generated statically linked Linux x86-64 NASM file\n");
    fwWriteChunkOrCrash(&asmWriter, "; compile and link with the following command:\n");
    fwWriteChunkOrCrash(&asmWriter, "; `nasm -felf64 -o %s.o %s.asm && ld -o %s %s.o`\n", filename, filename, filename, filename);
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
        // verifyConditional(statement, symbolTable, asmWriter);
        assert(0 && "conditional generation not implemented yet");
    }
    else {
        generateStatement(statement, symbolTable, asmWriter);
    }
    generateStatements(nextStatement, symbolTable, asmWriter);
}

void generateStatement(AST* wrapper, HashMap* symbolTable, FileWriter* asmWriter) {
    AST* statement = wrapper->left;
    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        // verifyConditional(wrapper, symbolTable, asmWriter);
        assert(0 && "conditional generation not implemented yet");
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
        generateExpression(rval, symbolTable, asmWriter);
        size_t tmpAddr = rspOffset;
        rspOffset = stackTop;
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_ASSIGN\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rax, QWORD [rsp-%d]\n", tmpAddr);
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rsp-%d], rax\n", offset);
    }
    else {
        generateExpression(statement, symbolTable, asmWriter);
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
        arguments[numArgs++] = generateExpression(arg, symbolTable, asmWriter);
    }

    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTI\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, QWORD [rsp-%d]\n", arguments[0].offset);
        fwWriteChunkOrCrash(asmWriter, "  call puti\n");
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        fwWriteChunkOrCrash(asmWriter, "  ; CALL PUTI\n");
        fwWriteChunkOrCrash(asmWriter, "  mov rdi, QWORD [rsp-%d]\n", arguments[0].offset);
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
    (void) expression;
    (void) symbolTable;
    (void) asmWriter;

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
    else if (expression->kind == NODE_ADD) {
        fwWriteChunkOrCrash(asmWriter, "  ; NODE_ADD\n");
        Value a = generateExpression(expression->left, symbolTable, asmWriter);
        fwWriteChunkOrCrash(asmWriter, "  mov rcx, QWORD [rsp-%d]\n", a.offset);
        Value b = generateExpression(expression->right, symbolTable, asmWriter);
        fwWriteChunkOrCrash(asmWriter, "  mov rdx, QWORD [rsp-%d]\n", b.offset);
        fwWriteChunkOrCrash(asmWriter, "  add rcx, rdx\n");
        
        rspOffset += 8;
        fwWriteChunkOrCrash(asmWriter, "  mov QWORD [rsp-%d], rcx\n", rspOffset);

        Value lit = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = rspOffset
        };
        return lit;
    }

    fprintf(stderr, "%s UNIMPLEMENTED\n", nodeKindName(expression->kind));
    exit(1);
}
