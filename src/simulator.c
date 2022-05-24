#include "simulator.h"
#include "analyzer.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


size_t stackSize;
size_t stackTop;
uint8_t* stack;

// TODO: use sizeof ptr
#define MAX_ALIGN 8
size_t stackPush(size_t numBytes) {
    size_t start = stackTop;
    stackTop += numBytes;
    stackTop += MAX_ALIGN - ((stackTop-1) & (MAX_ALIGN-1)) - 1;
    assert(stackTop < stackSize-1);
    return start;
}

void stackPop(size_t restoreIdx) {
    restoreIdx += MAX_ALIGN - ((restoreIdx-1) & (MAX_ALIGN-1)) - 1;
    assert(restoreIdx <= stackTop);
    stackTop = restoreIdx;
}

uint8_t* valueAddr(Value val) {
    return stack + val.offset;
}

#define CAST_VALUE_RAW(value_type, value) *((value_type*) valueAddr(value))

#define BIND_CAST_VALUE(lvalue, cast_type, value) do { \
    switch ((value).type.kind) { \
        case TYPE_I64: lvalue = (cast_type) CAST_VALUE_RAW(int64_t, value); break; \
        case TYPE_U8: lvalue = (cast_type) CAST_VALUE_RAW(uint8_t, value); break; \
        case TYPE_F64: lvalue = (cast_type) CAST_VALUE_RAW(double, value); break; \
        default: lvalue = 0; assert(0); \
    } \
} while (0)


bool isTrue(Value val) {
    if (val.type.kind == TYPE_I64) {
        return CAST_VALUE_RAW(int64_t, val) != 0;
    }
    else if (val.type.kind == TYPE_U8) {
        return CAST_VALUE_RAW(uint8_t, val) != 0;
    }
    else if (val.type.kind == TYPE_F64) {
        return CAST_VALUE_RAW(double, val) != 0.0;
    }
    assert(0);
    return false;
}



void simulateProgram(AST* program, size_t maxStackSize) {
    stackSize = maxStackSize;
    stackTop = 0;
    stack = malloc(maxStackSize);

    HashMap symbolTable = hmNew(256);
    simulateStatements(program->right, &symbolTable);
    hmFree(symbolTable);
    free(stack);
}


void simulateStatements(AST* statement, HashMap* symbolTable) {
    if (statement->right == NULL) {
        return;
    }

    AST* toEval = statement->left;
    AST* nextStatement = statement->right;
    if (toEval->kind == NODE_WHILE || toEval->kind == NODE_IF) {
        simulateConditional(statement, symbolTable);
    }
    else {
        simulateStatement(statement, symbolTable);
    }
    simulateStatements(nextStatement, symbolTable);
}

void simulateConditional(AST* statement, HashMap* symbolTable) {
    AST* conditional = statement->left;

    AST* condition = conditional->left;
    if (conditional->kind == NODE_IF) {
        AST* left = conditional->left;
        bool hasElse = left->kind == NODE_ELSE; // left is `else`
        if (hasElse) {
            condition = left->left;
        }
        size_t stackPos = stackTop;
        Value result = evaluateExpression(condition, symbolTable);
        bool enterIf = isTrue(result);
        if (enterIf) {
            simulateBlock(conditional->right, symbolTable);
        }
        else if (hasElse) {
            simulateBlock(left->right, symbolTable);
        }
        stackPop(stackPos);
    }
    else if (conditional->kind == NODE_WHILE) {
        while (true) {
            size_t stackPos = stackTop;
            Value result = evaluateExpression(condition, symbolTable);
            if (!isTrue(result)) {
                stackPop(stackPos);
                break;
            }
            simulateBlock(conditional->right, symbolTable);
            stackPop(stackPos);
        }
    }
}

void simulateBlock(AST* body, HashMap* symbolTable) {
    // FIXME:
    // FIXME:
    // FIXME:
    // FIXME: THIS IS SO SLOW PLEASE FIX THIS
    // FIXME:
    // FIXME:
    // FIXME:
    // HashMap innerScope = hmCopy(*symbolTable);
    if (body->kind == NODE_BLOCK) {
        AST* first = body->right;
        // simulateStatements(first, &innerScope);
        simulateStatements(first, symbolTable);
    }
    else {
        simulateStatement(body, symbolTable);
    }
    // hmFree(innerScope);
}

void simulateStatement(AST* wrapper, HashMap* symbolTable) {
    AST* statement = wrapper->left;

    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        simulateConditional(wrapper, symbolTable);
    }
    else if (statement->kind == NODE_DEFINITION) {
        Token id = statement->token;
        AST* typeNode = statement->left;
        AST* subscript = typeNode->left;
        size_t arrSize = 0;
        bool hasSubscript = subscript != NULL;
        if (hasSubscript) { // array
            Value sizeExpr = evaluateExpression(subscript, symbolTable);
            // size should be an integral type
            BIND_CAST_VALUE(arrSize, size_t, sizeExpr);
        }
        
        if (arrSize == 0 && hasSubscript) {
            // TODO: throw a runtime error or something
            // illegal
            assert(0);
        }

        Token typeName = typeNode->token;
        TypeKind assignTypeKind = svCmp(svFromCStr("u8"), typeName.text) == 0 ? TYPE_U8 :
                                svCmp(svFromCStr("i64"), typeName.text) == 0 ? TYPE_I64 : TYPE_F64;
        Symbol newVar = (Symbol) {
            .id = id.text,
            .val = {
                .type = {
                    .kind = assignTypeKind,
                    .size = arrSize
                },
                .offset = stackPush(typeKindSize(assignTypeKind) * (hasSubscript ? arrSize : 1))
            }
        };
        hmPut(symbolTable, newVar);
    }
    else if (statement->kind == NODE_ASSIGN) {
        AST* lvalNode = statement->left;
        AST* rvalNode = statement->right;
        Value lval = evaluateExpression(lvalNode, symbolTable);
        Value rval = evaluateExpression(rvalNode, symbolTable);
        // NOTE: I don't think it's possible for rval to share memory with lval
        // memcpy is probably safe here, but use memmove here just to be certain
        memmove(valueAddr(lval), valueAddr(rval), typeSize(lval.type));
    }
    else {
        // everything else
        evaluateExpression(statement, symbolTable);
    }
}

Value evaluateCall(AST* call, HashMap* symbolTable) {

    AST* args = call->left;
    Value arguments[MAX_FUNC_ARGS];
    size_t numArgs = 0;

    for (AST* curArg = args;
         curArg->right != NULL;
         curArg = curArg->right)
    {
        AST* toCheck = curArg->left;
        arguments[numArgs++] = evaluateExpression(toCheck, symbolTable);
    }

    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        printf("%lld", (long long) CAST_VALUE_RAW(int64_t, arguments[0]));
    }
    else if (svCmp(svFromCStr("putf"), call->token.text) == 0) {
        printf("%f", CAST_VALUE_RAW(double, arguments[0]));
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        putchar(CAST_VALUE_RAW(uint8_t, arguments[0]));
    }
    else if (svCmp(svFromCStr("puts"), call->token.text) == 0) {
        printf("%s", valueAddr(arguments[0]));
    }
    else if (svCmp(svFromCStr("itof"), call->token.text) == 0) {
        Value ret = (Value) {
            .type = {
                .kind = TYPE_F64,
                .size = 0
            },
            .offset = stackPush(typeKindSize(TYPE_F64))
        };
        CAST_VALUE_RAW(double, ret) = (double) CAST_VALUE_RAW(int64_t, arguments[0]);
        return ret;
    }
    else if (svCmp(svFromCStr("ftoi"), call->token.text) == 0) {
        Value ret = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = stackPush(typeKindSize(TYPE_I64))
        };
        CAST_VALUE_RAW(int64_t, ret) = (int64_t) CAST_VALUE_RAW(double, arguments[0]);
        return ret;
    }
    else if (svCmp(svFromCStr("itoc"), call->token.text) == 0) {
        Value ret = (Value) {
            .type = {
                .kind = TYPE_U8,
                .size = 0
            },
            .offset = stackPush(typeKindSize(TYPE_I64))
        };
        CAST_VALUE_RAW(uint8_t, ret) = (uint8_t) CAST_VALUE_RAW(int64_t, arguments[0]);
        return ret;
    }
    else if (svCmp(svFromCStr("ctoi"), call->token.text) == 0) {
        Value ret = (Value) {
            .type = {
                .kind = TYPE_I64,
                .size = 0
            },
            .offset = stackPush(typeKindSize(TYPE_U8))
        };
        CAST_VALUE_RAW(int64_t, ret) = (int64_t) CAST_VALUE_RAW(uint8_t, arguments[0]);
        return ret;
    }
    else {
        assert(0 && "Unimplemented");
    }
    
    // unreachable
    return (Value) {
        .type = {
            .kind = TYPE_NONE,
            .size = 0
        },
        .offset = 0
    };
}

Value evaluateExpression(AST* expression, HashMap* symbolTable) {
    // NOTE: this function pushes temporaries to the stack
    // this is obviously not ideal, but it's simpler to implement for now

    if (expression->kind == NODE_STRING ||
            expression->kind == NODE_CHAR ||
            expression->kind == NODE_INTEGER ||
            expression->kind == NODE_FLOAT) {
        TypeKind kind = expression->kind == NODE_STRING ? TYPE_STR :
                        expression->kind == NODE_CHAR ? TYPE_U8 :
                        expression->kind == NODE_INTEGER ? TYPE_I64 : TYPE_F64;
        Value lit = (Value) {
            .type = {
                .kind = kind,
                .size = 0
            },
            .offset = stackPush(typeKindSize(kind))
        };
        if (expression->kind == NODE_STRING) {
            // FIXME: check for off by one errors here
            StringView src = expression->token.text;
            stackPush(src.size);
            size_t end;
            // possibly redundant copy
            assert(svToCStr(expression->token.text, &CAST_VALUE_RAW(char, lit), &end));
            // shrink extra space used by escape chars
            stackPop(stackTop - (src.size - end));
        }
        else if (expression->kind == NODE_CHAR) {
            char ch;
            assert(svToCStr(expression->token.text, &ch, NULL));
            CAST_VALUE_RAW(uint8_t, lit) = (uint8_t) ch;
        }
        else if (expression->kind == NODE_INTEGER) {
            CAST_VALUE_RAW(int64_t, lit) = svParseI64(expression->token.text);
        }
        else {
            CAST_VALUE_RAW(double, lit) = svParseF64(expression->token.text);
        }
        return lit;
    }
    else if (expression->kind == NODE_CALL) {
        return evaluateCall(expression, symbolTable);
    }
    else if (expression->kind == NODE_LVALUE) {
        Token id = expression->token;
        Symbol* var = hmGet(symbolTable, id.text);

        AST* subscript = expression->left;
        bool hasSubscript = subscript != NULL;
        bool varIsScalar = var->val.type.size == 0;
        size_t arrIndex = 0;
        if (hasSubscript) {
            Value indexExpr = evaluateExpression(subscript, symbolTable);
            // index should be an integral type
            BIND_CAST_VALUE(arrIndex, size_t, indexExpr);
        }
        size_t resultSize = (!varIsScalar && !hasSubscript) ? var->val.type.size : 0;
        TypeKind resultKind = var->val.type.kind;
        return (Value) {
            .type = {
                .kind = resultKind,
                .size = resultSize
            },
            .offset = var->val.offset + typeKindSize(resultKind) * arrIndex
        };
    }

    // if here, expression is an operator
    // as noted in analyzer, only operate on scalar types for now

    if (expression->right == NULL) {
        // unary operator

        Value operand = evaluateExpression(expression->left, symbolTable);
        TypeKind resultKind = unaryResultTypeKind(operand.type.kind, expression->kind);
        Value result = (Value) {
            .type = {
                .kind = resultKind,
                .size = 0
            },
            .offset = stackPush(typeKindSize(resultKind))
        };
#define UNARY_KIND_SWITCH_CASE(typeKind, type, unaryOp) \
        case typeKind: CAST_VALUE_RAW(type, result) = unaryOp CAST_VALUE_RAW(type, operand); break;
#define UNARY_SWITCH_CASE(opKind, unaryOp) \
        case opKind: \
            switch (operand.type.kind) { \
                UNARY_KIND_SWITCH_CASE(TYPE_I64, int64_t, unaryOp) \
                UNARY_KIND_SWITCH_CASE(TYPE_U8, uint8_t, unaryOp) \
                UNARY_KIND_SWITCH_CASE(TYPE_F64, double, unaryOp) \
                default: assert(0); \
            } break;

        switch (expression->kind) {
            UNARY_SWITCH_CASE(NODE_NEG, -)
            UNARY_SWITCH_CASE(NODE_NOT, !)
            default: assert(0);
        }

        return result;
#undef UNARY_KIND_SWITCH_CASE
#undef UNARY_SWITCH_CASE
    }
    else {
        // binary operator

        Value lOperand = evaluateExpression(expression->left, symbolTable);
        Value rOperand = evaluateExpression(expression->right, symbolTable);
        TypeKind resultKind = binaryResultTypeKind(lOperand.type.kind, rOperand.type.kind, expression->kind);
        Value result = (Value) {
            .type = {
                .kind = resultKind,
                .size = 0
            },
            .offset = stackPush(typeKindSize(resultKind))
        };

        // nasty trick, see note in `binaryResultTypeKind`
        if (lOperand.type.kind < rOperand.type.kind) {
            Value tmp = lOperand;
            lOperand = rOperand;
            rOperand = tmp;
        }
        // 'greater' type is lOperand


        // I am deeply ashamed of this code ...

#define BINARY_ROP_SWITCH_CASE(rOperandKind, lType, rType, binaryOp) \
        case rOperandKind: { \
            lType lOpRes; \
            BIND_CAST_VALUE(lOpRes, lType, lOperand); \
            rType rOpRes; \
            BIND_CAST_VALUE(rOpRes, rType, rOperand); \
            CAST_VALUE_RAW(lType, result) = (lType) (lOpRes binaryOp rOpRes); \
        } break;

#define BINARY_LOP_SWITCH_CASE_REAL(lOperandKind, lType, binaryOp) \
        case lOperandKind: \
            switch (lOperand.type.kind) { \
                BINARY_ROP_SWITCH_CASE(TYPE_I64, lType, int64_t, binaryOp) \
                BINARY_ROP_SWITCH_CASE(TYPE_U8, lType, uint8_t, binaryOp) \
                BINARY_ROP_SWITCH_CASE(TYPE_F64, lType, double, binaryOp) \
                default: assert(0); \
            } break;
#define BINARY_SWITCH_CASE_REAL(opKind, binaryOp) \
        case opKind: \
            switch (lOperand.type.kind) { \
                BINARY_LOP_SWITCH_CASE_REAL(TYPE_I64, int64_t, binaryOp) \
                BINARY_LOP_SWITCH_CASE_REAL(TYPE_U8, uint8_t, binaryOp) \
                BINARY_LOP_SWITCH_CASE_REAL(TYPE_F64, double, binaryOp) \
                default: assert(0); \
            } break;

#define BINARY_LOP_SWITCH_CASE_INTEGRAL(lOperandKind, lType, binaryOp) \
        case lOperandKind: \
            switch (lOperand.type.kind) { \
                BINARY_ROP_SWITCH_CASE(TYPE_I64, lType, int64_t, binaryOp) \
                BINARY_ROP_SWITCH_CASE(TYPE_U8, lType, uint8_t, binaryOp) \
                default: assert(0); \
            } break;
#define BINARY_SWITCH_CASE_INTEGRAL(opKind, binaryOp) \
        case opKind: \
            switch (lOperand.type.kind) { \
                BINARY_LOP_SWITCH_CASE_INTEGRAL(TYPE_I64, int64_t, binaryOp) \
                BINARY_LOP_SWITCH_CASE_INTEGRAL(TYPE_U8, uint8_t, binaryOp) \
                default: assert(0); \
            } break;

        switch (expression->kind) {
            BINARY_SWITCH_CASE_REAL(NODE_EQ, ==)
            BINARY_SWITCH_CASE_REAL(NODE_NE, !=)
            BINARY_SWITCH_CASE_REAL(NODE_GE, >=)
            BINARY_SWITCH_CASE_REAL(NODE_GT, >)
            BINARY_SWITCH_CASE_REAL(NODE_LE, <=)
            BINARY_SWITCH_CASE_REAL(NODE_LT, <)
            BINARY_SWITCH_CASE_REAL(NODE_AND, &&)
            BINARY_SWITCH_CASE_REAL(NODE_OR, ||)
            BINARY_SWITCH_CASE_REAL(NODE_ADD, +)
            BINARY_SWITCH_CASE_REAL(NODE_SUB, -)
            BINARY_SWITCH_CASE_REAL(NODE_MUL, *)
            BINARY_SWITCH_CASE_REAL(NODE_DIV, /)
            BINARY_SWITCH_CASE_INTEGRAL(NODE_MOD, %)
            default: assert(0);
        }
        // printf("%d\n", CAST_VALUE(uint8_t, result));
        return result;
    }
    // unreachable
}
