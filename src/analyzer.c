#include "analyzer.h"

#include <stdio.h>
#include <assert.h>

#include <string.h>
#include <stdlib.h>

const char* typeKindName(Type type) {
    static_assert(TYPE_COUNT == 4, "Exhaustive check of type kinds failed");
    const char* TypeKindNames[4] = {
        "TYPE_NONE",
        "TYPE_U8",
        "TYPE_I64",
        "TYPE_F64",
    };
    return TypeKindNames[type];
}

void verifyProgram(AST* program) {
    // TODO
    assert(program != NULL);
    assert(program->kind == NODE_PROGRAM);
}

void simulateProgram(AST* program) {
    HashMap symbolTable = hmNew(256);

    // TODO: scope
    // make copy of symbol table on each block recursion
    simulateStatements(program->right, &symbolTable);

    // printf("DONE\n");
    // Pair* a = hmGet(&symbolTable, svFromCStr("a"));
    // if (a->type == TYPE_I64) {
    //     printf("%s a = %ld\n", typeKindName(a->type), a->value->i64);
    // }
    // else if (a->type == TYPE_F64) {
    //     printf("%s a = %f\n", typeKindName(a->type), a->value->f64);
    // }
    // else if (a->type == TYPE_U8) {
    //     printf("%s a = '%c'\n", typeKindName(a->type), a->value->u8);
    // }

    // Pair* b = hmGet(&symbolTable, svFromCStr("b"));
    // if (b->type == TYPE_I64) {
    //     printf("%s b = %ld\n", typeKindName(b->type), b->value->i64);
    // }
    // else if (b->type == TYPE_F64) {
    //     printf("%s b = %f\n", typeKindName(b->type), b->value->f64);
    // }
    // else if (b->type == TYPE_U8) {
    //     printf("%s b = '%c'\n", typeKindName(b->type), b->value->u8);
    // }

    // Pair* c = hmGet(&symbolTable, svFromCStr("c"));
    // if (c->type == TYPE_I64) {
    //     printf("%s c = %ld\n", typeKindName(c->type), c->value->i64);
    // }
    // else if (c->type == TYPE_F64) {
    //     printf("%s c = %f\n", typeKindName(c->type), c->value->f64);
    // }
    // else if (c->type == TYPE_U8) {
    //     printf("%s c = '%c'\n", typeKindName(c->type), c->value->u8);
    // }

    hmFree(symbolTable);
}

/*
program SIMULATE STATEMENTS
statement SIMULATE STATEMENT
statement SIMULATE STATEMENT
statement SIMULATE STATEMENT
statement SIMULATE STATEMENT
statement SIMULATE STATEMENT
if  l -> condition SIMULATE CONDITIONAL
    r -> SIMULATE STATEMENTS
    statement SIMULATE STATEMENT
    statement SIMULATE STATEMENT
    statement SIMULATE STATEMENT
    statement SIMULATE STATEMENT
*/


// TODO: this too
#define VALUE_SCRATCH_SIZE 1024
Value valueScratchBuffer[VALUE_SCRATCH_SIZE];
size_t valueScratchBufferSize = 0;
Value* newValue() {
    assert(valueScratchBufferSize < VALUE_SCRATCH_SIZE-1);
    Value* val = valueScratchBuffer + valueScratchBufferSize++;
    return val;
}

Value* copyValue(Value* val) {
    Value* copy = newValue();
    memcpy(copy, val, sizeof(Value));
    return copy;
}

int dep = 0;
void simulateStatements(AST* statement, HashMap* symbolTable) {
    assert(statement != NULL);
    if (statement->right == NULL) {
        return;
    }

    AST* toEval = statement->left;
    assert(toEval != NULL);

    // TODO: subroutine body
    // TODO: if, else
    /*
        if (a) b;
        if (x) if (y) z;
        if (c) {
            d; e;
        }
    */
    if (toEval->kind == NODE_WHILE || toEval->kind == NODE_IF) {
        // conditional
        simulateConditional(toEval, symbolTable);
    }
    else {
        simulateStatement(toEval, symbolTable);
    }
    AST* nextStatement = statement->right;
    simulateStatements(nextStatement, symbolTable);
}

void simulateConditional(AST* conditional, HashMap* symbolTable) {
    // for (int i = 0; i < dep; ++i)
    //     putchar('\t');
    // printf("<%s: \""SV_FMT"\">\n", nodeKindName(conditional->kind), SV_ARG(conditional->token.text));
    dep += 1;
    AST* body = conditional->right; assert(body != NULL);
    AST* condition = conditional->left; assert(condition != NULL);
    while (true) {
        Expression conditionResult = evaluateExpression(condition, symbolTable);
        bool validCondition = conditionResult.arrSize == 0/* && (conditionResult.type == TYPE_I64 || conditionResult.type == TYPE_U8)*/;
        if (!validCondition) {
            // TODO: actual fail at token error message function/macro with filename
            printf("ERROR: CONDITION WAS NOT A SCALAR VALUE\n");
            exit(1);
        }
        bool evalTrue = (conditionResult.type == TYPE_I64 && conditionResult.value->i64 != 0) ||
                        (conditionResult.type == TYPE_U8 && conditionResult.value->u8 != 0) ||
                        (conditionResult.type == TYPE_F64 && conditionResult.value->f64 != 0);
        if (!evalTrue) {
            break;
        }
        if (body->kind == NODE_BLOCK) {
            AST* first = body->right;
            simulateStatements(first, symbolTable);
        }
        else {
            // TODO
            // assert(0);
            simulateStatement(body, symbolTable);
        }
    }
    dep -= 1;
}

void simulateStatement(AST* statement, HashMap* symbolTable) {
    assert(statement != NULL);
    if (statement->kind != NODE_WHILE && statement->kind != NODE_IF) {
        // for (int i = 0; i < dep; ++i)
        //     putchar('\t');
        // printf("<%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
    }
    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        simulateConditional(statement, symbolTable);
    }
    else if (statement->kind == NODE_DEFINITION) {
        // printf("DEFINITION\n");
        Token id = statement->token;
        assert(statement->left != NULL);
        Token type = statement->left->token;
        AST* index = statement->right; // can be NULL
        Expression size = {0};
        if (index != NULL) {
            assert(0 && "Arrays are not implemented yet");
            // TODO: parse expression and assert is positive integer
            size = evaluateExpression(index, symbolTable);
            if (size.type != TYPE_I64) {
                // TODO: actual fail at token error message function/macro with filename
                printf("ERROR: ARRAY SIZE MUST BE INTEGER \n");
                exit(1);
            }
        }
        // TODO: determine type enum value during tokenizing rather than here
        // this is a wasteful computation
        Type assignType = svCmp(svFromCStr("u8"), type.text) == 0 ? TYPE_U8 :
                        svCmp(svFromCStr("i64"), type.text) == 0 ? TYPE_I64 :
                        // svCmp(svFromCStr("u64"), type.text) == 0 ? TYPE_U64 :
                        TYPE_F64;
        
        Symbol* var = hmGet(symbolTable, id.text);
        if (var != NULL) {
            // variable already exists
            // TODO: actual fail at token error message function/macro with filename
            printf("ERROR: REDEFINITION OF VARIABLE "SV_FMT"\n", SV_ARG(id.text));
            exit(1);
        }
        Symbol newVar = (Symbol) {
            .id = id.text,
            .type = assignType,
            .arrSize = index == NULL ? 0 : size.value->i64, // 0 if scalar type
            .value = newValue(), // TODO: allocate this properly
        };
        hmPut(symbolTable, newVar);
    }
    else if (statement->kind == NODE_ASSIGN) {
        AST* lval = statement->left;
        assert(lval->kind == NODE_LVALUE);
        Symbol* var = hmGet(symbolTable, lval->token.text);
        if (var == NULL) {
            printf("ERROR: ASSIGN TO UNDEFINED VARIABLE "SV_FMT"\n", SV_ARG(lval->token.text));
            exit(1);
        }
        Expression result = evaluateExpression(statement->right, symbolTable);
        if (var->type != result.type) {
            // TODO: better error
            printf("ERROR: ASSIGNMENT MISMATCHED TYPES "SV_FMT"\n", SV_ARG(lval->token.text));
            exit(1);
        }
        var->value = result.value;
    }
    else {
        // TODO: expressions
        // printf("\t\tUNHANDLED STATEMENT <%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
        evaluateExpression(statement, symbolTable);
    }
}

Expression evaluateCall(AST* call, HashMap* symbolTable) {
    (void) symbolTable;
    // TODO: user defined subroutines
    assert(call != NULL);
    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        AST* args = call->left; assert(args != NULL);
        AST* arg1 = args->left; assert(arg1 != NULL);
        Expression arg1val = evaluateExpression(arg1, symbolTable);
        assert(arg1val.type == TYPE_I64);
        printf("%ld", arg1val.value->i64);
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        AST* args = call->left; assert(args != NULL);
        AST* arg1 = args->left; assert(arg1 != NULL);
        Expression arg1val = evaluateExpression(arg1, symbolTable);
        assert(arg1val.type == TYPE_U8);
        printf("%c", arg1val.value->u8);
    }
    else {
        printf("ERROR: UNKNOWN FUNCTION\n");
        exit(1);
    }
    return (Expression) { TYPE_NONE }; // return value
}

Expression evaluateExpression(AST* expression, HashMap* symbolTable) {
    assert(expression != NULL);
    // terminal nodes evaluate self
    if (expression->kind == NODE_INTEGER) {
        Value* val = newValue();
        val->i64 = svParseI64(expression->token.text);
        // TODO: check if parse was actually successful
        return (Expression) {
            .type = TYPE_I64,
            .arrSize = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_FLOAT) {
        Value* val = newValue();
        val->f64 = svParseF64(expression->token.text);
        // TODO: check if parse was actually successful
        return (Expression) {
            .type = TYPE_F64,
            .arrSize = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_CHAR) {
        Value* val = newValue();
        val->u8 = expression->token.text.data[0];
        if (val->u8 == '\\') {
            assert(expression->token.text.size > 1);
            switch (expression->token.text.data[1]) {
                case 'n': val->u8 = '\n'; break;
                case 't': val->u8 = '\t'; break;
                case '\\': val->u8 = '\\'; break;
                default: {
                    printf("ERROR: Unhandled escape character '%c'\n", val->u8);
                    exit(1);
                }
            }
        }
        // TODO: check if parse was actually successful
        return (Expression) {
            .type = TYPE_U8,
            .arrSize = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_CALL) {
        return evaluateCall(expression, symbolTable);
    }
    if (expression->kind == NODE_STRING) {
        assert(0 && "Strings are not implemented yet");
    }
    // printf("EVALUATING NODE: %s => <%s: \""SV_FMT"\">\n", nodeKindName(expression->kind), tokenKindName(expression->token.kind), SV_ARG(expression->token.text));
    if (expression->kind == NODE_LVALUE) {
        assert(expression->left == NULL && "Arrays not implemented yet");
        Symbol* var = hmGet(symbolTable, expression->token.text);
        if (var == NULL) {
            // variable does not exist
            // TODO: actual fail at token error message function/macro with filename
            printf("ERROR: USE OF UNDEFINED VARIABLE "SV_FMT"\n", SV_ARG(expression->token.text));
            exit(1);
        }
        return (Expression) {
            .type = var->type,
            .arrSize = 0, // only scalar type for now
            .value = copyValue(var->value),
        };
    }

    // TODO: float, ...
    int64_t result;
    if (expression->right == NULL) {
        // unary
        Expression l = evaluateExpression(expression->left, symbolTable);
        if (!(l.type == TYPE_U8 || l.type == TYPE_I64)) {
            printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
            exit(1);
        }
        int64_t leftVal = l.type == TYPE_U8 ? l.value->u8 : l.value->i64;
        switch (expression->kind) {
            case NODE_NOT: result = (!leftVal); break;
            case NODE_NEG: result = (-leftVal); break;
            default: assert(0 && "Unhandled expression kind");
        }
    }
    else {
        // binary
        Expression l = evaluateExpression(expression->left, symbolTable);
        Expression r = evaluateExpression(expression->right, symbolTable);
        if (!(l.type == TYPE_U8 || l.type == TYPE_I64) ||
            !(r.type == TYPE_U8 || r.type == TYPE_I64)) {
            printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
            exit(1);
        }
        int64_t leftVal = l.type == TYPE_U8 ? l.value->u8 : l.value->i64;
        int64_t rightVal = r.type == TYPE_U8 ? r.value->u8 : r.value->i64;
        switch (expression->kind) {
            case NODE_EQ: result = (leftVal == rightVal); break;
            case NODE_NE: result = (leftVal != rightVal); break;
            case NODE_GE: result = (leftVal >= rightVal); break;
            case NODE_GT: result = (leftVal > rightVal); break;
            case NODE_LE: result = (leftVal <= rightVal); break;
            case NODE_LT: result = (leftVal < rightVal); break;
            case NODE_AND: result = (leftVal && rightVal); break;
            case NODE_OR: result = (leftVal || rightVal); break;
            case NODE_ADD: result = (leftVal + rightVal); break;
            case NODE_SUB: result = (leftVal - rightVal); break;
            case NODE_MUL: result = (leftVal * rightVal); break;
            case NODE_DIV: result = (leftVal / rightVal); break;
            case NODE_MOD: result = (leftVal % rightVal); break;
            default: assert(0 && "Unhandled expression kind");
        }
    }
    
    Value* val = newValue();
    val->i64 = result;
    return (Expression) {
        .type = TYPE_I64,
        .arrSize = 0,
        .value = val,
    };
}
