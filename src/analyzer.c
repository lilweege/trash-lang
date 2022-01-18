#include "analyzer.h"
#include "compiler.h"

#include <stdio.h>
#include <assert.h>

#include <string.h>
#include <stdlib.h>


bool isIntegral(TypeKind kind) {
    return kind == TYPE_I64 || kind == TYPE_U8;
}

bool isScalar(Type type) {
    return type.size == 0;
}


TypeKind unaryResultTypeKind(TypeKind type, NodeKind op) {
    if (type == TYPE_NONE || type == TYPE_STR) {
        return TYPE_NONE;
    }
    switch (op) {
        case NODE_NEG: // NOTE: neg unsigned is allowed
        case NODE_NOT:
            return type;
        default: break;
    }
    return TYPE_NONE; // unsupported
}

TypeKind binaryResultTypeKind(TypeKind type1, TypeKind type2, NodeKind op) {
    if (type1 == TYPE_NONE || type2 == TYPE_NONE ||
        type1 == TYPE_STR || type2 == TYPE_STR) {
        // cannot operate on none type (void) or string literal
        return TYPE_NONE;
    }

    // this is a nasty trick which greatly reduces the combinatorial explosion in simulator.c (yay macros)
    // of course all binary operations must be commutative
    // additionally, it depends on the order of the TypeKind enum in conjunction with the fact that 
    // according to my upcasting rules, the max of both type enums is the type to cast to
    // this will almost definitely break when more types are introduced
    static_assert(TYPE_COUNT == 5, "Exhaustive check of type kinds failed");

    if (type1 < type2) {
        TypeKind t = type1;
        type1 = type2;
        type2 = t;
    }
    assert(type1 >= type2);


    switch (op) {
        case NODE_EQ:
        case NODE_NE:
        case NODE_GE:
        case NODE_GT:
        case NODE_LE:
        case NODE_LT:
        case NODE_AND:
        case NODE_OR:
            return TYPE_U8;
        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV: {
            return type1; // NASTY TRICK...

            // if (type1 == TYPE_F64 || type2 == TYPE_F64)
            //     return TYPE_F64;
            // if (type1 == TYPE_I64 || type2 == TYPE_I64)
            //     return TYPE_I64;
            // if (type1 == TYPE_U8 || type2 == TYPE_U8)
            //     return TYPE_U8;
            // return TYPE_NONE; // probably unreachable
        }
        case NODE_MOD: {
            if (type1 == TYPE_F64 || type2 == TYPE_F64)
                return TYPE_NONE;

            return type1;
            // if (type1 == TYPE_I64 || type2 == TYPE_I64)
            //     return TYPE_I64;
            // if (type1 == TYPE_U8 || type2 == TYPE_U8)
            //     return TYPE_U8;
            // return TYPE_NONE; // probably unreachable
        }
        default: break;
    }
    return TYPE_NONE; // unsupported
}


const char* typeKindName(TypeKind type) {
    static_assert(TYPE_COUNT == 5, "Exhaustive check of type kinds failed");
    const char* TypeKindNames[5] = {
        "TYPE_NONE",
        "TYPE_STR",
        "TYPE_U8",
        "TYPE_I64",
        "TYPE_F64",
    };
    return TypeKindNames[type];
}

const char* typeKindKeyword(TypeKind type) {
    static_assert(TYPE_COUNT == 5, "Exhaustive check of type kinds failed");
    const char* TypeKindKeywords[5] = {
        "void",
        "str",
        "u8",
        "i64",
        "f64",
    };
    return TypeKindKeywords[type];
}

void verifyProgram(const char* filename, AST* program) {
    assert(program != NULL);
    assert(program->kind == NODE_PROGRAM);

    HashMap symbolTable = hmNew(256);
    verifyStatements(filename, program->right, &symbolTable);
    hmFree(symbolTable);
}

void verifyStatements(const char* filename, AST* statement, HashMap* symbolTable) {
    assert(statement != NULL);
    if (statement->right == NULL) {
        return;
    }

    AST* toCheck = statement->left;
    AST* nextStatement = statement->right;
    if (toCheck->kind == NODE_WHILE || toCheck->kind == NODE_IF) {
        verifyConditional(filename, statement, symbolTable);
    }
    else {
        verifyStatement(filename, statement, symbolTable);
    }
    verifyStatements(filename, nextStatement, symbolTable);
}

void verifyConditional(const char* filename, AST* statement, HashMap* symbolTable) {
    AST* conditional = statement->left;
    assert(conditional != NULL);

    AST* condition = conditional->left;
    if (conditional->kind == NODE_ELSE) {
        condition = NULL;
    }
    else if (conditional->kind == NODE_IF) {
        AST* left = conditional->left;
        assert(left != NULL);
        if (left->kind == NODE_ELSE) {
            verifyConditional(filename, conditional, symbolTable);
            condition = left->left;
        }
    }

    if (condition != NULL) {
        Type conditionType = checkExpression(filename, condition, symbolTable);
        if (!isScalar(conditionType) || (!isIntegral(conditionType.kind) && conditionType.kind != TYPE_F64)) {
            // invalid type (void, string, array)
            compileError((FileInfo) { filename, condition->token.pos },
                         "Condition was not a scalar value");
        }
    }
    
    AST* body = conditional->right;
    assert(body != NULL);
    HashMap innerScope = hmCopy(*symbolTable);
    if (body->kind == NODE_BLOCK) {
        AST* first = body->right;
        verifyStatements(filename, first, &innerScope);
    }
    else {
        verifyStatement(filename, body, symbolTable);
    }
    hmFree(innerScope);
}


void verifyStatement(const char* filename, AST* wrapper, HashMap* symbolTable) {
    AST* statement = wrapper->left;
    assert(statement != NULL);
    if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
        verifyConditional(filename, wrapper, symbolTable);
    }
    else if (statement->kind == NODE_DEFINITION) {
        Token id = statement->token;
        AST* typeNode = statement->left;
        assert(typeNode != NULL);
        AST* subscript = typeNode->left;
        bool hasSubscript = subscript != NULL;
        if (hasSubscript) {
            // defining an array
            Type sizeExpr = checkExpression(filename, subscript, symbolTable);
            if (!isScalar(sizeExpr) || !isIntegral(sizeExpr.kind)) {
                compileError((FileInfo) { filename, subscript->token.pos },
                             "Array size must be an integal type");
            }
        }
        
        // TODO: determine type enum value during tokenizing rather than here
        // this is a wasteful computation
        Token typeName = typeNode->token;
        TypeKind assignTypeKind = svCmp(svFromCStr("u8"), typeName.text) == 0 ? TYPE_U8 :
                                svCmp(svFromCStr("i64"), typeName.text) == 0 ? TYPE_I64 : TYPE_F64;
        
        Symbol* var = hmGet(symbolTable, id.text);
        if (var != NULL) {
            compileError((FileInfo) { filename, id.pos },
                         "Redefinition of variable \""SV_FMT"\"",
                         SV_ARG(id.text));
        }

        Symbol newVar = (Symbol) {
            .id = id.text,
            .val = {
                .type = {
                    .kind = assignTypeKind,
                    .size = hasSubscript ? 1 : 0
                },
            }
        };
        hmPut(symbolTable, newVar);
    }
    else if (statement->kind == NODE_ASSIGN) {
        AST* lval = statement->left;
        AST* rval = statement->right;
        assert(lval != NULL);
        assert(lval->kind == NODE_LVALUE);
        Token id = lval->token;
        Symbol* var = hmGet(symbolTable, id.text);
        if (var == NULL) {
            // undefined variable
            compileError((FileInfo) { filename, statement->token.pos },
                         "Cannot assign to undefined variable \""SV_FMT"\"",
                         SV_ARG(id.text));
        }

        // check expression type match declared type
        Type lvalType = checkExpression(filename, lval, symbolTable);
        Type rvalType = checkExpression(filename, rval, symbolTable);
        if (!isScalar(lvalType)) {
            // currently the only valid non-scalar assignment is strings
            if (lvalType.kind == TYPE_U8 && rvalType.kind == TYPE_STR) {
                return;
            }
            compileError((FileInfo) { filename, statement->token.pos },
                         "Illegal assignment to array \""SV_FMT"\"",
                         SV_ARG(lval->token.text));
        }
        
        if (!isScalar(rvalType)) {
            compileError((FileInfo) { filename, statement->token.pos },
                         "Illegal assignment of array \""SV_FMT"\"",
                         SV_ARG(rval->token.text));
        }

        // both scalar types
        if (lvalType.kind != rvalType.kind) {
            // mismatched types
            compileError((FileInfo) { filename, lval->token.pos },
                         "Cannot assign \""SV_FMT"\" due to mismatched types",
                         SV_ARG(lval->token.text));
        }
        // assignment ok
    }
    else {
        // everything else
        checkExpression(filename, statement, symbolTable);
    }
}

Type checkCall(const char* filename, AST* call, HashMap* symbolTable) {
    // TODO: user defined subroutines
    assert(call != NULL);
    
    AST* args = call->left; assert(args != NULL);
    Type argTypes[MAX_FUNC_ARGS];
    size_t numArgs = 0;

    for (AST* curArg = args;
         curArg->right != NULL;
         curArg = curArg->right)
    {
        assert(numArgs < MAX_FUNC_ARGS - 1);
        AST* toCheck = curArg->left;
        argTypes[numArgs++] = checkExpression(filename, toCheck, symbolTable);
    }


#define expectNumArgs(expected) do {                                                                      \
        if (numArgs != expected) {                                                                        \
            compileError((FileInfo) { filename, call->token.pos },                                        \
                         "Call to \""SV_FMT"\": Incorrect number of arguments: expected %d, found %d",    \
                         SV_ARG(call->token.text),                                                        \
                         expected, numArgs);                                                              \
        }                                                                                                 \
    } while (0)

#define argMatch(position, expectedKind, expectedScalar) \
        (argTypes[position].kind == expectedKind && \
         isScalar(argTypes[position]) == expectedScalar)

#define positionalArgFail(position, expectedKind, expectedScalar) do { \
        FileInfo info = (FileInfo) { filename, call->token.pos }; \
        const char* errorFmt = expectedScalar ? \
                isScalar(argTypes[position]) ? \
                    "Call to \""SV_FMT"\": Type mismatch at position %zu: expected %s, found %s" : \
                    "Call to \""SV_FMT"\": Type mismatch at position %zu: expected %s, found %s[]" : \
                isScalar(argTypes[position]) ? \
                    "Call to \""SV_FMT"\": Type mismatch at position %zu: expected %s[], found %s" : \
                    "Call to \""SV_FMT"\": Type mismatch at position %zu: expected %s[], found %s[]" ; \
        compileError(info, \
                     errorFmt, \
                     SV_ARG(call->token.text), \
                     position, \
                     typeKindKeyword(expectedKind), \
                     typeKindKeyword(argTypes[position].kind)); \
    } while (0)


    // built in functions
    if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
        expectNumArgs(1);
        if (!argMatch(0, TYPE_I64, true)) {
            positionalArgFail(0, TYPE_I64, true);
        }
        return (Type) { .kind = TYPE_NONE, .size = 0 };
    }
    if (svCmp(svFromCStr("putf"), call->token.text) == 0) {
        expectNumArgs(1);
        if (!argMatch(0, TYPE_F64, true)) {
            positionalArgFail(0, TYPE_F64, true);
        }
        return (Type) { .kind = TYPE_NONE, .size = 0 };
    }
    if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
        expectNumArgs(1);
        if (!argMatch(0, TYPE_U8, true)) {
            positionalArgFail(0, TYPE_U8, true);
        }
        return (Type) { .kind = TYPE_NONE, .size = 0 };
    }
    if (svCmp(svFromCStr("puts"), call->token.text) == 0) {
        expectNumArgs(1);
        if (!argMatch(0, TYPE_U8, false) &&
            !argMatch(0, TYPE_STR, true)) {
            positionalArgFail(0, TYPE_U8, false);
        }
        return (Type) { .kind = TYPE_NONE, .size = 0 };
    }
    
    // TODO: type cast functions
    // TODO: user defined subroutines
    compileError((FileInfo) { filename, call->token.pos },
                    "Call to unknown function \""SV_FMT"\"",
                    SV_ARG(call->token.text));

    return (Type) { 0 }; // unreachable
#undef expectNumArgs
#undef argMatch
#undef positionalArgFail
}

Type checkExpression(const char* filename, AST* expression, HashMap* symbolTable) {
    assert(expression != NULL);
    if (expression->kind == NODE_INTEGER) {
        return (Type) { .kind = TYPE_I64, .size = 0 };
    }
    if (expression->kind == NODE_FLOAT) {
        return (Type) { .kind = TYPE_F64, .size = 0 };
    }
    if (expression->kind == NODE_CHAR) {
        return (Type) { .kind = TYPE_U8, .size = 0 };
    }
    if (expression->kind == NODE_STRING) {
        return (Type) { .kind = TYPE_STR, .size = 0 };
    }
    if (expression->kind == NODE_CALL) {
        return checkCall(filename, expression, symbolTable);
    }
    if (expression->kind == NODE_LVALUE) {
        Token id = expression->token;
        Symbol* var = hmGet(symbolTable, id.text);
        if (var == NULL) {
            // undefined variable
            compileError((FileInfo) { filename, id.pos },
                         "Use of undefined variable \""SV_FMT"\"",
                         SV_ARG(id.text));
        }

        AST* subscript = expression->left;
        bool hasSubscript = subscript != NULL;
        bool varIsScalar = var->val.type.size == 0;
        if (hasSubscript) {
            // array subscript
            if (varIsScalar) {
                compileError((FileInfo) { filename, subscript->token.pos },
                             "Cannot take subscript of scalar type");
            }
            Type indexExpr = checkExpression(filename, subscript, symbolTable);
            if (!isScalar(indexExpr) || !isIntegral(indexExpr.kind)) {
                compileError((FileInfo) { filename, subscript->token.pos },
                             "Array index must be an integal type");
            }
        }
        
        // result type size
        // array
            // no subscript -> array
            // subscript -> scalar
        // scalar
            // no subscript -> scalar
            // subscript -> illegal
        return (Type) {
            .kind = var->val.type.kind,
            .size = (!varIsScalar && !hasSubscript) ? 1 : 0,
        };
    }

    // if here, expression is an operator

    TypeKind resultTypeKind;
    if (expression->right == NULL) {
        // unary operator
        Type resultType = checkExpression(filename, expression->left, symbolTable);
        if (!isScalar(resultType)) {
            // NOTE: for now, forbid any operators on non scalar types
            // this will change when pointer operators are introduced
            compileError((FileInfo) { filename, expression->token.pos },
                         "Unsupported operator "SV_FMT" between non-scalar types",
                         SV_ARG(expression->token.text));
        }
        resultTypeKind = unaryResultTypeKind(resultType.kind, expression->kind);
    }
    else {
        // binary operator
        Type lType = checkExpression(filename, expression->left, symbolTable);
        Type rType = checkExpression(filename, expression->right, symbolTable);
        if (!isScalar(lType) || !isScalar(rType)) {
            // NOTE: for now, forbid any operators on non scalar types
            // this will change when pointer operators are introduced
            compileError((FileInfo) { filename, expression->token.pos },
                         "Unsupported operator "SV_FMT" between non-scalar types",
                         SV_ARG(expression->token.text));
        }
        resultTypeKind = binaryResultTypeKind(lType.kind, rType.kind, expression->kind);
        if (resultTypeKind == TYPE_NONE) {
            compileError((FileInfo) { filename, expression->token.pos },
                         "Unsupported operator "SV_FMT" between types %s and %s",
                         SV_ARG(expression->token.text),
                         typeKindKeyword(lType.kind),
                         typeKindKeyword(rType.kind));
        }
    }
    
    return (Type) {
        .kind = resultTypeKind,
        .size = 0,
    };
}



// void simulateProgram(const char* filename, AST* program) {
//     HashMap symbolTable = hmNew(256);

//     // TODO: scope
//     // make copy of symbol table on each block recursion
//     simulateStatements(filename, program->right, &symbolTable);

//     hmFree(symbolTable);
// }

// /*
// program SIMULATE STATEMENTS
// statement SIMULATE STATEMENT
// statement SIMULATE STATEMENT
// statement SIMULATE STATEMENT
// statement SIMULATE STATEMENT
// statement SIMULATE STATEMENT
// if  l -> condition SIMULATE CONDITIONAL
//     r -> SIMULATE STATEMENTS
//     statement SIMULATE STATEMENT
//     statement SIMULATE STATEMENT
//     statement SIMULATE STATEMENT
//     statement SIMULATE STATEMENT
// */


// // TODO: this too
// #define VALUE_SCRATCH_SIZE (1<<18)
// Value valueScratchBuffer[VALUE_SCRATCH_SIZE];
// size_t valueScratchBufferSize = 0;
// Value* newValue(size_t num) {
//     assert(valueScratchBufferSize < VALUE_SCRATCH_SIZE-num);
//     Value* val = valueScratchBuffer + valueScratchBufferSize;
//     valueScratchBufferSize += num;
//     return val;
// }

// int dep = 0;
// void simulateStatements(const char* filename, AST* statement, HashMap* symbolTable) {
//     assert(statement != NULL);
//     if (statement->right == NULL) {
//         return;
//     }

//     AST* toEval = statement->left;
//     assert(toEval != NULL);

//     // TODO: subroutine body
//     // TODO: if, else
//     /*
//         if (a) b;
//         if (x) if (y) z;
//         if (c) {
//             d; e;
//         }
//     */
//     if (toEval->kind == NODE_WHILE || toEval->kind == NODE_IF) {
//         // conditional
//         simulateConditional(filename, toEval, symbolTable);
//     }
//     else {
//         simulateStatement(filename, toEval, symbolTable);
//     }
//     AST* nextStatement = statement->right;
//     simulateStatements(filename, nextStatement, symbolTable);
// }

// void simulateConditional(const char* filename, AST* conditional, HashMap* symbolTable) {
//     // for (int i = 0; i < dep; ++i)
//     //     putchar('\t');
//     // printf("<%s: \""SV_FMT"\">\n", nodeKindName(conditional->kind), SV_ARG(conditional->token.text));
//     dep += 1;
//     AST* body = conditional->right; assert(body != NULL);
//     AST* condition = conditional->left; // assert(condition != NULL); // else fails
//     while (true) {
//         Expression conditionResult = evaluateExpression(filename, condition, symbolTable);
//         // if (conditionResult.index != -1) {
//             // compileError((FileInfo) { filename, condition->token.pos },
//             //              "Condition was not a scalar value");
//         // }
//         bool evalTrue = (conditionResult.type == TYPE_I64 && conditionResult.value[conditionResult.index].i64 != 0) ||
//                         (conditionResult.type == TYPE_U8 && conditionResult.value[conditionResult.index].u8 != 0) ||
//                         (conditionResult.type == TYPE_F64 && conditionResult.value[conditionResult.index].f64 != 0);
//         if (!evalTrue) {
//             break;
//         }
//         if (body->kind == NODE_BLOCK) {
//             AST* first = body->right;
//             simulateStatements(filename, first, symbolTable);
//         }
//         else {
//             // TODO
//             // assert(0);
//             simulateStatement(filename, body, symbolTable);
//         }
//         if (conditional->kind == NODE_IF) {
//             break;
//         }
//     }
//     dep -= 1;
// }

// void simulateStatement(const char* filename, AST* statement, HashMap* symbolTable) {
//     assert(statement != NULL);
//     if (statement->kind != NODE_WHILE && statement->kind != NODE_IF) {
//         // for (int i = 0; i < dep; ++i)
//         //     putchar('\t');
//         // printf("<%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
//     }
//     if (statement->kind == NODE_WHILE || statement->kind == NODE_IF) {
//         simulateConditional(filename, statement, symbolTable);
//     }
//     else if (statement->kind == NODE_DEFINITION) {
//         // printf("DEFINITION\n");
//         Token id = statement->token;
//         assert(statement->left != NULL);
//         Token type = statement->left->token;
//         AST* index = statement->left->left; // can be NULL
//         int64_t arrSize = -1;
        
//         if (index != NULL) {
//             Expression size = evaluateExpression(filename, index, symbolTable);
//             if (!isIntegral(size.type)) {
//                 compileError((FileInfo) { filename, type.pos },
//                              "Array size must be an integal type");
//             }
//             if (size.type == TYPE_I64) {
//                 arrSize = (int64_t) size.value[size.index].i64;
//             }
//             else {
//                 arrSize = (int64_t) size.value[size.index].u8;
//             }
//         }
//         // TODO: determine type enum value during tokenizing rather than here
//         // this is a wasteful computation
//         TypeKind assignTypeKind = svCmp(svFromCStr("u8"), type.text) == 0 ? TYPE_U8 :
//                         svCmp(svFromCStr("i64"), type.text) == 0 ? TYPE_I64 :
//                         // svCmp(svFromCStr("u64"), type.text) == 0 ? TYPE_U64 :
//                         TYPE_F64;
        
//         Symbol* var = hmGet(symbolTable, id.text);
//         if (var != NULL) {
//             compileError((FileInfo) { filename, id.pos },
//                          "Redefinition of variable \""SV_FMT"\"", SV_ARG(id.text));
//         }
//         // printf("DEFINING VAR "SV_FMT" with arrSize=%ld\n", SV_ARG(id.text), arrSize);
//         Symbol newVar = (Symbol) {
//             .id = id.text,
//             .type = assignTypeKind,
//             .arrSize = arrSize,
//             .value = newValue(arrSize < 0 ? 1 : arrSize), // TODO: allocate this properly
//         };
//         hmPut(symbolTable, newVar);
//     }
//     else if (statement->kind == NODE_ASSIGN) {
//         AST* lval = statement->left;
//         assert(lval->kind == NODE_LVALUE);
//         Symbol* symbol = hmGet(symbolTable, lval->token.text);
//         if (symbol == NULL) {
//             compileError((FileInfo) { filename, lval->token.pos },
//                          "Cannot assign to undefined variable \""SV_FMT"\"", SV_ARG(lval->token.text));
//         }
//         // if (lval->left == NULL && symbol->arrSize != -1) {
//         //     // no lval subscript and symbol is array
//         //     compileError((FileInfo) { filename, lval->token.pos },
//         //                  "Cannot assign value to array \""SV_FMT"\"", SV_ARG(lval->token.text));
//         // }
//         if (lval->left != NULL && symbol->arrSize == -1) {
//             // TODO: move this error to eval expr
//             compileError((FileInfo) { filename, lval->token.pos },
//                          "Cannot take subscript of scalar value \""SV_FMT"\"", SV_ARG(lval->token.text));
//         }
//         Expression result = evaluateExpression(filename, statement->right, symbolTable);
//         // special case string
//         bool stringAssign = symbol->type == TYPE_U8 && symbol->arrSize != -1 && result.type == TYPE_STR;
//         if (symbol->type != result.type && !stringAssign) {
//             // TODO: better error
//             compileError((FileInfo) { filename, lval->token.pos },
//                          "Cannot assign \""SV_FMT"\" due to mismatched types", SV_ARG(lval->token.text));
//             exit(1);
//         }
//         // ...
//         if (stringAssign) {
//             size_t numEscapeChars = 0;
//             size_t sz = result.value->sv.size;
//             for (size_t i = 0; i < sz; ++i) {
//                 char ch = result.value->sv.data[i];
//                 if (ch == '\\') {
//                     ++numEscapeChars;
//                     if (++i >= sz) {
//                         break;
//                     }
//                     char escapeChar = result.value->sv.data[i];
//                     switch (escapeChar) {
//                         case '"': ch = '"'; break;
//                         case 'n': ch = '\n'; break;
//                         case 't': ch = '\t'; break;
//                         default: assert(0 && "Unhandled escape character");
//                     }
//                 }
//                 symbol->value[i - numEscapeChars].u8 = ch;
//             }
//             symbol->value[sz - numEscapeChars].u8 = 0;
//         }
//         else if (symbol->arrSize == -1) { // scalar
//             // assert(result.index == -1);
//             symbol->value[0] = result.value[result.index];
//         }
//         else {
//             assert(lval->left != NULL);
//             Expression index = evaluateExpression(filename, lval->left, symbolTable);
//             if (!isIntegral(index.type)) {
//                 compileError((FileInfo) { filename, lval->left->token.pos },
//                              "Array index must be an integral type", SV_ARG(lval->left->token.text));
//             }
//             int64_t idx;
//             if (index.type == TYPE_I64) {
//                 idx = (int64_t) index.value[index.index].i64;
//             }
//             else {
//                 idx = (int64_t) index.value[index.index].u8;
//             }

//             // printf("%ld, %ld\n", idx, result.index);
//             // assert();
//             symbol->value[idx] = result.value[result.index];
//         }
//     }
//     else {
//         // TODO: expressions
//         // printf("\t\tUNHANDLED STATEMENT <%s: \""SV_FMT"\">\n", nodeKindName(statement->kind), SV_ARG(statement->token.text));
//         evaluateExpression(filename, statement, symbolTable);
//     }
// }

// Expression evaluateCall(const char* filename, AST* call, HashMap* symbolTable) {
//     // TODO: user defined subroutines
//     assert(call != NULL);
//     if (svCmp(svFromCStr("puti"), call->token.text) == 0) {
//         AST* args = call->left; assert(args != NULL);
//         AST* arg1 = args->left; assert(arg1 != NULL);
//         Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
//         assert(arg1val.type == TYPE_I64);
//         printf("%ld", arg1val.value[arg1val.index].i64);
//     }
//     else if (svCmp(svFromCStr("putc"), call->token.text) == 0) {
//         AST* args = call->left; assert(args != NULL);
//         AST* arg1 = args->left; assert(arg1 != NULL);
//         Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
//         assert(arg1val.type == TYPE_U8);
//         putchar(arg1val.value[arg1val.index].u8);
//     }
//     else if (svCmp(svFromCStr("puts"), call->token.text) == 0) {
//         AST* args = call->left; assert(args != NULL);
//         AST* arg1 = args->left; assert(arg1 != NULL);
//         Expression arg1val = evaluateExpression(filename, arg1, symbolTable);
//         if (arg1val.type == TYPE_U8) {
//             for (size_t i = 0; arg1val.value[i].u8; ++i) {
//                 putchar(arg1val.value[i].u8);
//             }
//         }
//         else if (arg1val.type == TYPE_STR) {
//             // printf(SV_FMT, SV_ARG(arg1val.value->sv));
//             char buf[128];
//             if (svToCStr(arg1val.value->sv, buf, NULL)) {
//                 printf("%s", buf);
//             }
//         }
//     }
//     else {
//         printf("ERROR: UNKNOWN FUNCTION\n");
//         exit(1);
//     }
//     return (Expression) { TYPE_NONE }; // return value
// }

// Expression evaluateExpression(const char* filename, AST* expression, HashMap* symbolTable) {
//     assert(expression != NULL);
//     // terminal nodes evaluate self
//     if (expression->kind == NODE_INTEGER) {
//         Value* val = newValue(1);
//         val->i64 = svParseI64(expression->token.text);
//         // TODO: check if parse was actually successful
//         return (Expression) {
//             .type = TYPE_I64,
//             .index = 0,
//             .value = val,
//         };
//     }
//     if (expression->kind == NODE_FLOAT) {
//         Value* val = newValue(1);
//         val->f64 = svParseF64(expression->token.text);
//         // TODO: check if parse was actually successful
//         return (Expression) {
//             .type = TYPE_F64,
//             .index = 0,
//             .value = val,
//         };
//     }
//     if (expression->kind == NODE_CHAR) {
//         Value* val = newValue(1);
//         val->u8 = expression->token.text.data[0];
//         if (val->u8 == '\\') {
//             assert(expression->token.text.size > 1);
//             switch (expression->token.text.data[1]) {
//                 case 'n': val->u8 = '\n'; break;
//                 case 't': val->u8 = '\t'; break;
//                 case '\\': val->u8 = '\\'; break;
//                 default: {
//                     printf("ERROR: Unhandled escape character '%c'\n", val->u8);
//                     exit(1);
//                 }
//             }
//         }
//         // TODO: check if parse was actually successful
//         return (Expression) {
//             .type = TYPE_U8,
//             .index = 0,
//             .value = val,
//         };
//     }
//     if (expression->kind == NODE_CALL) {
//         return evaluateCall(filename, expression, symbolTable);
//     }
//     if (expression->kind == NODE_STRING) {
//         // assert(0 && "Strings are not implemented yet");
//         // TODO: escape characters
//         Value* val = newValue(1);
//         val->sv = expression->token.text;
//         return (Expression) {
//             .type = TYPE_STR,
//             .index = 0,
//             .value = val,
//         };
//     }
//     if (expression->kind == NODE_LVALUE) {
//         int64_t index = 0;
//         // printf("EVALUATING NODE: %s => <%s: \""SV_FMT"\">\n", nodeKindName(expression->kind), tokenKindName(expression->token.kind), SV_ARG(expression->token.text));
//         if (expression->left != NULL) {
//             // printf("expression->left = %p\n", (void*) expression->left);
//             // array subscript
//             Expression size = evaluateExpression(filename, expression->left, symbolTable);
//             if (!isIntegral(size.type)) {
//                 compileError((FileInfo) { filename, expression->left->token.pos },
//                              "Array index must be an integral type", SV_ARG(expression->token.text));
//             }
//             if (size.type == TYPE_I64) {
//                 index = (int64_t) size.value[size.index].i64;
//             }
//             else {
//                 index = (int64_t) size.value[size.index].u8;
//             }
//         }
//         // printf("index = %ld\n", index);
//         Symbol* var = hmGet(symbolTable, expression->token.text);
//         if (var == NULL) {
//             // variable does not exist
//             compileError((FileInfo) { filename, expression->token.pos },
//                          "Use of undefined variable \""SV_FMT"\"", SV_ARG(expression->token.text));
//         }
//         return (Expression) {
//             .type = var->type,
//             .index = index,
//             .value = var->value,
//         };
//     }

//     // else operator

//     // TODO: float, ...
//     int64_t result;
//     if (expression->right == NULL) {
//         // unary
//         Expression l = evaluateExpression(filename, expression->left, symbolTable);
//         if (!isIntegral(l.type)) {
//             printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
//             exit(1);
//         }
//         int64_t leftVal = l.type == TYPE_U8 ? l.value[l.index].u8 : l.value[l.index].i64;
//         switch (expression->kind) {
//             case NODE_NOT: result = (!leftVal); break;
//             case NODE_NEG: result = (-leftVal); break;
//             default: assert(0 && "Unhandled expression kind");
//         }
//     }
//     else {
//         // binary
//         Expression l = evaluateExpression(filename, expression->left, symbolTable);
//         Expression r = evaluateExpression(filename, expression->right, symbolTable);
//         if (!isIntegral(l.type) || !isIntegral(r.type)) {
//             printf("ERROR: INVALID TYPE FOR BINARY OPERATOR\n");
//             exit(1);
//         }
//         int64_t leftVal = l.type == TYPE_U8 ? l.value[l.index].u8 : l.value[l.index].i64;
//         int64_t rightVal = r.type == TYPE_U8 ? r.value[r.index].u8 : r.value[r.index].i64;
//         switch (expression->kind) {
//             case NODE_EQ: result = (leftVal == rightVal); break;
//             case NODE_NE: result = (leftVal != rightVal); break;
//             case NODE_GE: result = (leftVal >= rightVal); break;
//             case NODE_GT: result = (leftVal > rightVal); break;
//             case NODE_LE: result = (leftVal <= rightVal); break;
//             case NODE_LT: result = (leftVal < rightVal); break;
//             case NODE_AND: result = (leftVal && rightVal); break;
//             case NODE_OR: result = (leftVal || rightVal); break;
//             case NODE_ADD: result = (leftVal + rightVal); break;
//             case NODE_SUB: result = (leftVal - rightVal); break;
//             case NODE_MUL: result = (leftVal * rightVal); break;
//             case NODE_DIV: result = (leftVal / rightVal); break;
//             case NODE_MOD: result = (leftVal % rightVal); break;
//             default: assert(0 && "Unhandled expression kind");
//         }
//     }
    
//     Value* val = newValue(1); // BAD!
//     val->i64 = result;
//     return (Expression) {
//         .type = TYPE_I64,
//         .index = 0,
//         .value = val,
//     };
// }
