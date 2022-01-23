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
            return TYPE_I64;
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
        verifyStatement(filename, body, &innerScope);
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

            // TODO: compile time evaluations of contants ?
            if (subscript->kind != NODE_INTEGER) {
                compileError((FileInfo) { filename, subscript->token.pos },
                             "Array size must be constant");
            }

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
