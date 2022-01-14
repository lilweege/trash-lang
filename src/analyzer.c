#include "analyzer.h"
#include "compiler.h"

#include <stdio.h>
#include <assert.h>

#include <string.h>
#include <stdlib.h>


bool isIntegral(Type type) {
    return type == TYPE_I64 || type == TYPE_U8;
}

Type unaryResultType(Type type, NodeKind op) {
    switch (op) {
        case NODE_NEG: // NOTE: neg unsigned is allowed
        case NODE_NOT:
            return type == TYPE_STR ?
                    TYPE_NONE : TYPE_U8;
        default: break;
    }
    return TYPE_NONE; // unsupported
}

Type binaryResultType(Type type1, Type type2, NodeKind op) {
    if (type1 == TYPE_NONE || type2 == TYPE_NONE) {
        // cannot operate on none type (void)
        return TYPE_NONE;
    }

    // all operations are commutative
    // therefore only consider one triangle of matrix
    // reduce combinatorial explosion
    if (type1 > type2) {
        Type t = type1;
        type1 = type2;
        type2 = t;
    }
    assert(type2 >= type1);

    static_assert(TYPE_COUNT == 5, "Exhaustive check of type kinds failed");

    switch (op) {
        case NODE_EQ:
        case NODE_NE:
        case NODE_GE:
        case NODE_GT:
        case NODE_LE:
        case NODE_LT:
        case NODE_AND:
        case NODE_OR:
            return (type1 == TYPE_STR || type2 == TYPE_STR) ?
                    TYPE_NONE : TYPE_U8;
        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV: {
            if (type1 == TYPE_STR || type2 == TYPE_STR)
                return TYPE_NONE;
            
            if (type1 == TYPE_F64 || type2 == TYPE_F64)
                return TYPE_F64;
            if (type1 == TYPE_I64 || type2 == TYPE_I64)
                return TYPE_I64;
            if (type1 == TYPE_U8 || type2 == TYPE_U8)
                return TYPE_U8;
            return TYPE_NONE; // probably unreachable
        }
        case NODE_MOD: {
            if (type1 == TYPE_STR || type2 == TYPE_STR)
                return TYPE_NONE;
            if (type1 == TYPE_F64 || type2 == TYPE_F64)
                return TYPE_NONE;
            
            if (type1 == TYPE_I64 || type2 == TYPE_I64)
                return TYPE_I64;
            if (type1 == TYPE_U8 || type2 == TYPE_U8)
                return TYPE_U8;
            return TYPE_NONE; // probably unreachable
        }
        default: break;
    }
    return TYPE_NONE; // unsupported
}


const char* typeKindName(Type type) {
    static_assert(TYPE_COUNT == 5, "Exhaustive check of type kinds failed");
    const char* TypeKindNames[5] = {
        "TYPE_STR",
        "TYPE_NONE",
        "TYPE_U8",
        "TYPE_I64",
        "TYPE_F64",
    };
    return TypeKindNames[type];
}

void verifyProgram(const char* filename, AST* program) {
    assert(program != NULL);
    assert(program->kind == NODE_PROGRAM);

    HashMap symbolTable = hmNew(256);
    verifyStatements(filename, program->right, &symbolTable);

    hmFree(symbolTable);
}

void verifyStatements(const char* filename, AST* statement, HashMap* symbolTable) {
    // ...
}


void simulateProgram(const char* filename, AST* program) {
    HashMap symbolTable = hmNew(256);

    // TODO: scope
    // make copy of symbol table on each block recursion
    simulateStatements(filename, program->right, &symbolTable);

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
#define VALUE_SCRATCH_SIZE (1<<18)
Value valueScratchBuffer[VALUE_SCRATCH_SIZE];
size_t valueScratchBufferSize = 0;
Value* newValue(size_t num) {
    assert(valueScratchBufferSize < VALUE_SCRATCH_SIZE-num);
    Value* val = valueScratchBuffer + valueScratchBufferSize;
    valueScratchBufferSize += num;
    return val;
}

int dep = 0;
void simulateStatements(const char* filename, AST* statement, HashMap* symbolTable) {
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
        simulateConditional(filename, toEval, symbolTable);
    }
    else {
        simulateStatement(filename, toEval, symbolTable);
    }
    AST* nextStatement = statement->right;
    simulateStatements(filename, nextStatement, symbolTable);
}

void simulateConditional(const char* filename, AST* conditional, HashMap* symbolTable) {
    // for (int i = 0; i < dep; ++i)
    //     putchar('\t');
    // printf("<%s: \""SV_FMT"\">\n", nodeKindName(conditional->kind), SV_ARG(conditional->token.text));
    dep += 1;
    AST* body = conditional->right; assert(body != NULL);
    AST* condition = conditional->left; assert(condition != NULL);
    while (true) {
        Expression conditionResult = evaluateExpression(filename, condition, symbolTable);
        // if (conditionResult.index != -1) {
            // compileError((FileInfo) { filename, condition->token.pos },
            //              "Condition was not a scalar value");
        // }
        bool evalTrue = (conditionResult.type == TYPE_I64 && conditionResult.value[conditionResult.index].i64 != 0) ||
                        (conditionResult.type == TYPE_U8 && conditionResult.value[conditionResult.index].u8 != 0) ||
                        (conditionResult.type == TYPE_F64 && conditionResult.value[conditionResult.index].f64 != 0);
        if (!evalTrue) {
            break;
        }
        if (body->kind == NODE_BLOCK) {
            AST* first = body->right;
            simulateStatements(filename, first, symbolTable);
        }
        else {
            // TODO
            // assert(0);
            simulateStatement(filename, body, symbolTable);
        }
        if (conditional->kind == NODE_IF) {
            break;
        }
    }
    dep -= 1;
}

void simulateStatement(const char* filename, AST* statement, HashMap* symbolTable) {
    assert(statement != NULL);
    if (statement->kind != NODE_WHILE && statement->kind != NODE_IF) {
        // for (int i = 0; i < dep; ++i)
        //     putchar('\t');
        // printf("<%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
    }
    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        simulateConditional(filename, statement, symbolTable);
    }
    else if (statement->kind == NODE_DEFINITION) {
        // printf("DEFINITION\n");
        Token id = statement->token;
        assert(statement->left != NULL);
        Token type = statement->left->token;
        AST* index = statement->left->left; // can be NULL
        int64_t arrSize = -1;
        
        if (index != NULL) {
            Expression size = evaluateExpression(filename, index, symbolTable);
            if (!isIntegral(size.type)) {
                compileError((FileInfo) { filename, type.pos },
                             "Array size must be an integal type");
            }
            if (size.type == TYPE_I64) {
                arrSize = (int64_t) size.value[size.index].i64;
            }
            else {
                arrSize = (int64_t) size.value[size.index].u8;
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
            compileError((FileInfo) { filename, id.pos },
                         "Redefinition of variable \""SV_FMT"\"", SV_ARG(id.text));
        }
        // printf("DEFINING VAR "SV_FMT" with arrSize=%ld\n", SV_ARG(id.text), arrSize);
        Symbol newVar = (Symbol) {
            .id = id.text,
            .type = assignType,
            .arrSize = arrSize,
            .value = newValue(arrSize < 0 ? 1 : arrSize), // TODO: allocate this properly
        };
        hmPut(symbolTable, newVar);
    }
    else if (statement->kind == NODE_ASSIGN) {
        AST* lval = statement->left;
        assert(lval->kind == NODE_LVALUE);
        Symbol* symbol = hmGet(symbolTable, lval->token.text);
        if (symbol == NULL) {
            compileError((FileInfo) { filename, lval->token.pos },
                         "Cannot assign to undefined variable \""SV_FMT"\"", SV_ARG(lval->token.text));
        }
        // if (lval->left == NULL && symbol->arrSize != -1) {
        //     // no lval subscript and symbol is array
        //     compileError((FileInfo) { filename, lval->token.pos },
        //                  "Cannot assign value to array \""SV_FMT"\"", SV_ARG(lval->token.text));
        // }
        if (lval->left != NULL && symbol->arrSize == -1) {
            // TODO: move this error to eval expr
            compileError((FileInfo) { filename, lval->token.pos },
                         "Cannot take subscript of scalar value \""SV_FMT"\"", SV_ARG(lval->token.text));
        }
        Expression result = evaluateExpression(filename, statement->right, symbolTable);
        // special case string
        bool stringAssign = symbol->type == TYPE_U8 && symbol->arrSize != -1 && result.type == TYPE_STR;
        if (symbol->type != result.type && !stringAssign) {
            // TODO: better error
            compileError((FileInfo) { filename, lval->token.pos },
                         "Cannot assign \""SV_FMT"\" due to mismatched types", SV_ARG(lval->token.text));
            exit(1);
        }
        // ...
        if (stringAssign) {
            size_t numEscapeChars = 0;
            size_t sz = result.value->sv.size;
            for (size_t i = 0; i < sz; ++i) {
                char ch = result.value->sv.data[i];
                if (ch == '\\') {
                    ++numEscapeChars;
                    if (++i >= sz) {
                        break;
                    }
                    char escapeChar = result.value->sv.data[i];
                    switch (escapeChar) {
                        case '"': ch = '"'; break;
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        default: assert(0 && "Unhandled escape character");
                    }
                }
                symbol->value[i - numEscapeChars].u8 = ch;
            }
            symbol->value[sz - numEscapeChars].u8 = 0;
        }
        else if (symbol->arrSize == -1) { // scalar
            // assert(result.index == -1);
            symbol->value[0] = result.value[result.index];
        }
        else {
            assert(lval->left != NULL);
            Expression index = evaluateExpression(filename, lval->left, symbolTable);
            if (!isIntegral(index.type)) {
                compileError((FileInfo) { filename, lval->left->token.pos },
                             "Array index must be an integral type", SV_ARG(lval->left->token.text));
            }
            int64_t idx;
            if (index.type == TYPE_I64) {
                idx = (int64_t) index.value[index.index].i64;
            }
            else {
                idx = (int64_t) index.value[index.index].u8;
            }

            // printf("%ld, %ld\n", idx, result.index);
            // assert();
            symbol->value[idx] = result.value[result.index];
        }
    }
    else {
        // TODO: expressions
        // printf("\t\tUNHANDLED STATEMENT <%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
        evaluateExpression(filename, statement, symbolTable);
    }
}

Expression evaluateCall(const char* filename, AST* call, HashMap* symbolTable) {
    // TODO: user defined subroutines
    assert(call != NULL);
    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        AST* args = call->left; assert(args != NULL);
        AST* arg1 = args->left; assert(arg1 != NULL);
        Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
        assert(arg1val.type == TYPE_I64);
        printf("%ld", arg1val.value[arg1val.index].i64);
    }
    else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        AST* args = call->left; assert(args != NULL);
        AST* arg1 = args->left; assert(arg1 != NULL);
        Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
        assert(arg1val.type == TYPE_U8);
        putchar(arg1val.value[arg1val.index].u8);
    }
    else if (svCmp(svFromCStr("puts"), call->token.text) == 0) {
        AST* args = call->left; assert(args != NULL);
        AST* arg1 = args->left; assert(arg1 != NULL);
        Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
        if (arg1val.type == TYPE_U8) {
            for (size_t i = 0; arg1val.value[i].u8; ++i) {
                putchar(arg1val.value[i].u8);
            }
        }
        else if (arg1val.type == TYPE_STR) {
            // printf(SV_FMT, SV_ARG(arg1val.value->sv));
            char buf[128];
            if (svToCStr(arg1val.value->sv, buf, NULL)) {
                printf("%s", buf);
            }
        }
    }
    else {
        printf("ERROR: UNKNOWN FUNCTION\n");
        exit(1);
    }
    return (Expression) { TYPE_NONE }; // return value
}

Expression evaluateExpression(const char* filename, AST* expression, HashMap* symbolTable) {
    assert(expression != NULL);
    // terminal nodes evaluate self
    if (expression->kind == NODE_INTEGER) {
        Value* val = newValue(1);
        val->i64 = svParseI64(expression->token.text);
        // TODO: check if parse was actually successful
        return (Expression) {
            .type = TYPE_I64,
            .index = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_FLOAT) {
        Value* val = newValue(1);
        val->f64 = svParseF64(expression->token.text);
        // TODO: check if parse was actually successful
        return (Expression) {
            .type = TYPE_F64,
            .index = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_CHAR) {
        Value* val = newValue(1);
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
            .index = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_CALL) {
        return evaluateCall(filename, expression, symbolTable);
    }
    if (expression->kind == NODE_STRING) {
        // assert(0 && "Strings are not implemented yet");
        // TODO: escape characters
        Value* val = newValue(1);
        val->sv = expression->token.text;
        return (Expression) {
            .type = TYPE_STR,
            .index = 0,
            .value = val,
        };
    }
    if (expression->kind == NODE_LVALUE) {
        int64_t index = 0;
        // printf("EVALUATING NODE: %s => <%s: \""SV_FMT"\">\n", nodeKindName(expression->kind), tokenKindName(expression->token.kind), SV_ARG(expression->token.text));
        if (expression->left != NULL) {
            // printf("expression->left = %p\n", (void*) expression->left);
            // array subscript
            Expression size = evaluateExpression(filename, expression->left, symbolTable);
            if (!isIntegral(size.type)) {
                compileError((FileInfo) { filename, expression->left->token.pos },
                             "Array index must be an integral type", SV_ARG(expression->token.text));
            }
            if (size.type == TYPE_I64) {
                index = (int64_t) size.value[size.index].i64;
            }
            else {
                index = (int64_t) size.value[size.index].u8;
            }
        }
        // printf("index = %ld\n", index);
        Symbol* var = hmGet(symbolTable, expression->token.text);
        if (var == NULL) {
            // variable does not exist
            compileError((FileInfo) { filename, expression->token.pos },
                         "Use of undefined variable \""SV_FMT"\"", SV_ARG(expression->token.text));
        }
        return (Expression) {
            .type = var->type,
            .index = index,
            .value = var->value,
        };
    }

    // else operator

    // TODO: float, ...
    int64_t result;
    if (expression->right == NULL) {
        // unary
        Expression l = evaluateExpression(filename, expression->left, symbolTable);
        if (!isIntegral(l.type)) {
            printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
            exit(1);
        }
        int64_t leftVal = l.type == TYPE_U8 ? l.value[l.index].u8 : l.value[l.index].i64;
        switch (expression->kind) {
            case NODE_NOT: result = (!leftVal); break;
            case NODE_NEG: result = (-leftVal); break;
            default: assert(0 && "Unhandled expression kind");
        }
    }
    else {
        // binary
        Expression l = evaluateExpression(filename, expression->left, symbolTable);
        Expression r = evaluateExpression(filename, expression->right, symbolTable);
        if (!isIntegral(l.type) || !isIntegral(r.type)) {
            printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
            exit(1);
        }
        int64_t leftVal = l.type == TYPE_U8 ? l.value[l.index].u8 : l.value[l.index].i64;
        int64_t rightVal = r.type == TYPE_U8 ? r.value[r.index].u8 : r.value[r.index].i64;
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
    
    Value* val = newValue(1); // BAD!
    val->i64 = result;
    return (Expression) {
        .type = TYPE_I64,
        .index = 0,
        .value = val,
    };
}
