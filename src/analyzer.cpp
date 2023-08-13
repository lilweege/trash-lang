#include "analyzer.hpp"
#include "compileerror.hpp"
#include "parser.hpp"

#include <cassert>

static bool IsPrimitiveType(Type type) {
    return type.typeName == "u8" || type.typeName == "i64" || type.typeName == "f64";
}

static bool IsIntegralType(Type type) {
    return type.typeName == "u8" || type.typeName == "i64";
}

static AccessKind TypeAccessKind(Type type) {
    bool isScalar = type.numPointerLevels == 0;
    bool isIntegral = IsIntegralType(type);
    return (isScalar || isIntegral) ? AccessKind::INTEGRAL : AccessKind::FLOATING;
}

static void AssertTypesAreSame(Type lhs, Type rhs, const Token& token) {
    if (lhs != rhs) {
        CompileErrorAt(token,
            "Incorrect type {}{:@>{}}, expected {}{:@>{}}",
            lhs.typeName, "", lhs.numPointerLevels,
            rhs.typeName, "", rhs.numPointerLevels);
    }
}

static int64_t RoudUpToPowerOfTwo(int64_t x, int64_t pow) {
    assert(pow && ((pow & (pow - 1)) == 0));
    return (x + pow - 1) & -pow;
}

Type Analyzer::VerifyType(ASTIndex typeIdx) {
    assert(typeIdx != AST_NULL);
    const ASTNode& type = ast.tree[typeIdx];
    const Token& typeName = tokens[type.tokenIdx];
    if (!resolvedTypes.contains(typeName.text))
        CompileErrorAt(typeName, "Unknown type \"{}\"", typeName.text);
    return Type{
        .typeName = typeName.text,
        .numPointerLevels = type.type.numPointerLevels,
    };
}

uint32_t Analyzer::TypeSize(Type type) {
    assert(resolvedTypes.contains(type.typeName));
    if (type.numPointerLevels != 0) return 8;
    return resolvedTypes[type.typeName].size;
}

uint32_t Analyzer::TypeAlign(Type type) {
    assert(resolvedTypes.contains(type.typeName));
    if (type.numPointerLevels != 0) return 8;
    return resolvedTypes[type.typeName].alignment;
}

void Analyzer::AddInstruction(Instruction ins) {
    if (keepGenerating)
        instructions.push_back(ins);
}

void Analyzer::AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident) {
    if (procedureDefns.contains(ident.text)) {
        // Variable name same as procedure name, not allowed
        CompileErrorAt(ident, "Local variable '{}' shadows procedure", ident.text);
    }
    else if (symbolTable.contains(ident.text)) {
        CompileErrorAt(ident, "Redefinition of variable '{}'", ident.text);
    }
}

void Analyzer::VerifyDefinition(ASTIndex defnIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    const ASTNode& stmt = ast.tree[defnIdx];
    const ASTNode::ASTDefinition& defn = stmt.defn;
    const Token& token = tokens[stmt.tokenIdx];
    std::string_view ident = token.text;
    const Token& asgnToken = tokens[ast.tree[defnIdx].tokenIdx + 1];

    bool hasInitExpr = defn.initExpr != AST_NULL;
    Type lhs = VerifyType(defn.type);

    size_t currStackSize = RoudUpToPowerOfTwo(blockStackSize.back(), TypeAlign(lhs));
    size_t offset = currStackSize;
    size_t typeSize = TypeSize(lhs);
    currStackSize += typeSize;

    if (hasInitExpr) {
        Type rhs = VerifyExpression(defn.initExpr, symbolTable);
        AssertTypesAreSame(lhs, rhs, asgnToken);
        size_t accessSize = typeSize;
        assert(IsPrimitiveType(lhs) || lhs.numPointerLevels != 0);
        assert(typeSize <= 8);
        AddInstruction(Instruction{.opcode=Instruction::Opcode::STORE_FAST, .fastAccess={.varAddr=offset,.accessSize=accessSize}});
    }

    AssertIdentUnusedInCurrentScope(symbolTable, token);

    stackAddrs.back()[ident] = offset;
    symbolTable[ident] = defnIdx;
    blockStackSize.back() = currStackSize;
    if (maxStaticStackSize < currStackSize)
        maxStaticStackSize = currStackSize;
}

#if 0
Type Analyzer::VerifyLValue(ASTIndex lvalIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable, bool isLoading) {
    assert(lvalIdx != AST_NULL);

    const Token& token = tokens[ast.tree[lvalIdx].tokenIdx];
    if (!symbolTable.contains(token.text)) {
        CompileErrorAt(token, "Use of undefined identifier '{}'", token.text);
    }

    // load/store fast if possible
    const ASTNode& expr = ast.tree[lvalIdx];
    bool hasSubscripts = expr.lvalue.subscripts != AST_EMPTY;
    bool hasMember = expr.lvalue.member != AST_NULL;
    if (!hasSubscripts && !hasMember) {
        ASTIndex defnIdx = symbolTable.at(tokens[expr.tokenIdx].text);
        Instruction::Opcode loadOrStore = isLoading ? Instruction::Opcode::LOAD_FAST : Instruction::Opcode::STORE_FAST;
        size_t varAddr = stackAddrs.back().at(token.text);
        Type varType = VerifyType(ast.tree[defnIdx].defn.type);
        size_t typeSize = TypeSize(varType);
        assert(IsPrimitiveType(varType) || varType.numPointerLevels != 0);
        assert(typeSize <= 8);
        AddInstruction(Instruction{.opcode=loadOrStore, .fastAccess={.varAddr=varAddr,.accessSize=typeSize}});
        return varType;
    }

    Instruction::Opcode loadOrStore = isLoading ? Instruction::Opcode::LOAD : Instruction::Opcode::STORE;
    // Evaluate left to right in loop:
    //   Subscripts chain (assert number of pointer levels) - each subscript loads with offset
    //   Member (assert name when accessing) - change base access offset

    ASTIndex memberIdx = lvalIdx;
    while (true) {
        const ASTNode& expr = ast.tree[memberIdx];
        const Token& exprToken = tokens[expr.tokenIdx];
        ASTIndex defnIdx = symbolTable.at(exprToken.text);
        Type varType = VerifyType(ast.tree[defnIdx].defn.type);

        bool hasSubscripts = expr.lvalue.subscripts != AST_EMPTY;
        bool hasMember = memberIdx != AST_NULL;
        // bool finalAccess = !hasSubscripts && !hasMember;
        // if (finalAccess) {
        //     // AddInstruction(Instruction{.opcode=loadOrStore, .access={.accessSize=,.offset=0,.hasSubscript=false}});
        //     break;
        // }

        // arr[x].bar[y].foo[a][b][c].baz = z
        //                                z
        // load_fast arr                  z arr
        // load_fast x                    z arr x
        // load_fast offset(bar)          z arr x offset(bar)
        // load TOP2+TOP1*size(bar*)+TOP  z &arr[x].bar
        // load_fast y                    z &arr[x] y
        // load_fast offset(foo)          z &arr[x] y offset(foo)
        // load TOP2+TOP1*size(bar)+TOP   z &arr[x][y].foo
        // load_fast a                    z &arr[x][y].foo a
        // load TOP1+TOP*size(foo**)      z &arr[x][y].foo[a]
        // load_fast b                    z &arr[x][y].foo[a] b
        // load TOP1+TOP*size(foo*)       z &arr[x][y].foo[a][b]
        // load_fast c                    z &arr[x][y].foo[a][b] c
        // load_fast offset(baz)          z &arr[x][y].foo[a][b] c offset(baz)
        // store TOP2+TOP1*size(foo)+TOP, TOP3

        // AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD, .access={.accessSize=8,.offset=0,.hasSubscript=true}});
        uint32_t numPointerLevels = varType.numPointerLevels;
        if (hasSubscripts) {
            const std::vector<ASTIndex>& subscripts = ast.lists[expr.lvalue.subscripts];
            for (ASTIndex subscript : subscripts) {
                Type subscriptType = VerifyExpression(subscript, symbolTable);
                if (subscriptType.numPointerLevels != 0 || !IsIntegralType(subscriptType)) {
                    CompileErrorAt(tokens[ast.tree[subscript].tokenIdx],
                        "Cannot subscript with non-scalar integral expression",
                        token.text);
                }

                if (numPointerLevels == 0) {
                    CompileErrorAt(tokens[ast.tree[subscript].tokenIdx],
                        "Cannot take subscript of scalar variable");
                }
                --numPointerLevels;

                // The last deref should account for member offset if applicable, wait until later
                if (numPointerLevels != 0) {
                    AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD, .access={.accessSize=8,.offset=0,.hasSubscript=true}});
                }
            }
        }

        const TypeDefn& typeDefn = resolvedTypes.at(exprToken.text);
        if (hasMember) {
            const ASTNode& member = ast.tree[expr.lvalue.member];
            const Token& memberName = tokens[member.tokenIdx];

            if (numPointerLevels != 0) {
                CompileErrorAt(memberName,
                    "Cannot access member of non-scalar type {}{:@>{}}",
                    varType.typeName, "", numPointerLevels);
            }

            // FIXME: Looping over all members is potentially slow for structs with many memebrs
            bool ok = false;
            size_t memberOffset;
            for (auto& [type, offset] : typeDefn.memberOffsets) {
                if (type.typeName == memberName.text) {
                    ok = true;
                    memberOffset = offset;
                    break;
                }
            }

            if (!ok) {
                CompileErrorAt(memberName, "Use of undefined member '{}' of struct '{}'",
                    memberName.text, tokens[expr.tokenIdx].text);
            }

            // AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD, .access={.accessSize=typeDefn.size,.offset=memberOffset,.hasSubscript=hasSubscripts}});
            memberIdx = member.lvalue.member;
        }
        else {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD, .access={.accessSize=typeDefn.size,.offset=0,.hasSubscript=hasSubscripts}});
        }
    }

    // return { defn.type, isScalar || hasSubscript };
    assert(0);
    return {};
}
#endif


// x = add(1, 2);

// 0: push 5        ; [] ... 5
// 1: save          ; [] ... 5 0
// 2: push 1        ; [] ... 5 0 1
// 3: push 2        ; [] ... 5 0 1 2
// 4: jmp add
// 5: ...           ; [] ... 3


// add(x, y) { z = x + y; ... return z; }

// 0: enter         ;  ... a b [x] y z    bp = ..; sp += ..
// ...              ;  ... a b [x] y z ...
// n: return        ;  ... a b []         tmp = TOP; sp = bp
//                  ; [] ... a            bp = TOP
//                  ; [] ... a z          TOP = tmp
//                  ; [] ... z a          swap
//                  ; [] ... z            jmp TOP


Type Analyzer::VerifyCall(ASTIndex callIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    const Token& callTok = tokens[ast.tree[callIdx].tokenIdx];
    const ASTNode::ASTCall& call = ast.tree[callIdx].call;
    const std::vector<ASTIndex>& args = ast.lists[call.args];
    const std::vector<ASTIndex>& targs = ast.lists[call.targs];
    size_t numArgs = args.size();
    size_t numTArgs = targs.size();

    auto AssertCorrectNumberOfTemplateArguments = [&](size_t numTParams) {
        if (numTArgs != numTParams) {
            if (numTParams == 1) {
                CompileErrorAt(callTok, "Expected {} template argument, got {} instead", numTParams, numTArgs);
            }
            else {
                CompileErrorAt(callTok, "Expected {} template arguments, got {} instead", numTParams, numTArgs);
            }
        }
    };

    auto AssertCorrectNumberOfArguments = [&](size_t numParams) {
        if (numArgs != numParams) {
            if (numParams == 1) {
                CompileErrorAt(callTok, "Expected {} argument, got {} instead", numParams, numArgs);
            }
            else {
                CompileErrorAt(callTok, "Expected {} arguments, got {} instead", numParams, numArgs);
            }
        }
    };

    auto AssertArgumentHasType = [&](size_t argIdx, Type paramType) {
        Type argType = VerifyExpression(args[argIdx], symbolTable);
        if (argType != paramType) {
            const Token& argTok = tokens[ast.tree[args[argIdx]].tokenIdx];
            CompileErrorAt(argTok, "Incompatible type {}{:@>{}} for argument {} of '{}' (expected {}{:@>{}})",
                           argType.typeName, "", argType.numPointerLevels,
                           argIdx+1,
                           callTok.text,
                           paramType.typeName, "", paramType.numPointerLevels);
        }
    };

    // Handle builtins
    if (callTok.text == "as" || callTok.text == "alloca" || callTok.text == "numof") {
        if (callTok.text == "alloca") {
            AssertCorrectNumberOfArguments(1);
            AssertCorrectNumberOfTemplateArguments(1);
            Type templateType = VerifyType(targs[0]);
            uint32_t typeSize = TypeSize(templateType);
            // Scale by the width of the array type (replace with a left shift when implemented)
            AssertArgumentHasType(0, templateType);
            AddInstruction(Instruction{.opcode=Instruction::Opcode::ALLOCA, .multiplier=typeSize});
            // Alloca for pointer is allowed
            return { templateType.typeName, templateType.numPointerLevels+1 };
        }
        else if (callTok.text == "as") {
            AssertCorrectNumberOfArguments(1);
            AssertCorrectNumberOfTemplateArguments(1);
            Type argType = VerifyExpression(args[0], symbolTable);
            Type templateType = VerifyType(targs[0]);

            // Allowed casts:
            //   - integral to integral
            //   - float to float
            //   - pointer to pointer
            //   - integral to pointer
            bool argTypeIsScalar = argType.numPointerLevels == 0;
            bool templateTypeIsScalar = templateType.numPointerLevels == 0;
            bool argTypeIsIntegral = IsIntegralType(argType);
            bool templateTypeIsIntegral = IsIntegralType(templateType);
            if ((argTypeIsIntegral || !argTypeIsScalar) != (templateTypeIsIntegral || !templateTypeIsScalar)) {
                const Token& argTok = tokens[ast.tree[args[0]].tokenIdx];
                CompileErrorAt(argTok, "Incompatible type {}{:@>{}} for template argument {}{:@>{}}) of '{}'",
                               argType.typeName, "", argType.numPointerLevels,
                               templateType.typeName, "", templateType.numPointerLevels,
                               callTok.text);
            }

            AddInstruction(Instruction{.opcode=Instruction::Opcode::UNARY_OP, .op={.op_kind=ASTKind::TRUNC_UNARYOP,.accessKind=TypeAccessKind(templateType)}});
            return templateType;
        }
        else {
            AssertCorrectNumberOfArguments(1);
            AssertCorrectNumberOfTemplateArguments(1);
            Type templateType = VerifyType(targs[0]);
            uint32_t typeSize = TypeSize(templateType);

            AssertArgumentHasType(0, Type{"i64", 0});
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind_=LiteralKind::I64,.i64=typeSize}});
            AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.op_kind=ASTKind::MUL_BINARYOP_EXPR,.accessKind=AccessKind::INTEGRAL}});

            return { "i64", 0 };
        }
    }
    else {
        // TODO: Constructors
        if (resolvedTypes.contains(callTok.text)) {
            assert(0);
        }

        // Push the address of the instruction following the jump
        size_t saveAddrIdx = instructions.size();
        AddInstruction(Instruction{.opcode=Instruction::Opcode::SAVE});

        if (!procedureDefns.contains(callTok.text)) {
            CompileErrorAt(callTok, "Call to undefined procedure '{}'", callTok.text);
        }

        const ProcedureDefn& defn = procedureDefns.at(callTok.text);
        size_t numParams = defn.paramTypes.size();
        AssertCorrectNumberOfArguments(numParams);
        AssertCorrectNumberOfTemplateArguments(0); // TODO: Allow user defined templates

        for (size_t argIdx{}; argIdx < numArgs; ++argIdx) {
            Type paramType = defn.paramTypes[argIdx];
            AssertArgumentHasType(argIdx, paramType);
            // TODO: For types that are passed by reference (for now, only arrays), check constness
        }

        // Defer resolving this address until all procedures have been generated
        unresolvedCalls.emplace_back(instructions.size(), callTok.text);
        AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});

        // Set the return address to be the next instruction
        instructions[saveAddrIdx].jmpAddr = instructions.size();

        return defn.returnType;
    }
}

Type Analyzer::VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    assert(exprIdx != AST_NULL);
    const ASTNode& expr = ast.tree[exprIdx];
    switch (expr.kind) {
        case ASTKind::LVALUE_EXPR:
            return VerifyLValue(exprIdx, symbolTable, true);
        case ASTKind::CALL_EXPR:
            return VerifyCall(exprIdx, symbolTable);
        case ASTKind::INTEGER_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind_=LiteralKind::I64,.i64=expr.literal.i64}});
            return { "i64", 0 };
        case ASTKind::FLOAT_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind_=LiteralKind::F64,.f64=expr.literal.f64}});
            return { "f64", 0 };
        case ASTKind::CHAR_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind_=LiteralKind::U8,.u8=expr.literal.u8}});
            return { "u8", 0 };
        case ASTKind::STRING_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind_=LiteralKind::STR,.str=expr.literal.str}});
            return { .typeName="u8", .numPointerLevels=1 };
        default: break;
    }

    // Otherwise, expect a binary or unary operator
    const Token& token = tokens[expr.tokenIdx];
    // Unary
    if (expr.kind == ASTKind::NEG_UNARYOP_EXPR ||
        expr.kind == ASTKind::NOT_UNARYOP_EXPR)
    {
        Type exprType = VerifyExpression(expr.unaryOp.expr, symbolTable);
        bool isScalar = exprType.numPointerLevels == 0;

        if (!isScalar) {
            CompileErrorAt(token, "Cannot apply unary operator to non-scalar type {}{:@>{}}",
                exprType.typeName, "", exprType.numPointerLevels);
        }
        if (!IsPrimitiveType(exprType)) {
            CompileErrorAt(token, "Cannot apply unary operator to non-primitive type {}", exprType.typeName);
        }
        if (expr.kind == ASTKind::NOT_UNARYOP_EXPR && !IsIntegralType(exprType)) {
            CompileErrorAt(token, "Cannot complement non-integral type {}", exprType.typeName);
        }

        AddInstruction(Instruction{.opcode=Instruction::Opcode::UNARY_OP, .op={.op_kind=expr.kind, .accessKind=TypeAccessKind(exprType)}});
        return exprType; // Result is same type
    }

    // Binary
    bool isLogical =
        expr.kind == ASTKind::EQ_BINARYOP_EXPR ||
        expr.kind == ASTKind::NE_BINARYOP_EXPR || 
        expr.kind == ASTKind::GE_BINARYOP_EXPR || 
        expr.kind == ASTKind::GT_BINARYOP_EXPR || 
        expr.kind == ASTKind::LE_BINARYOP_EXPR || 
        expr.kind == ASTKind::LT_BINARYOP_EXPR || 
        expr.kind == ASTKind::AND_BINARYOP_EXPR || 
        expr.kind == ASTKind::OR_BINARYOP_EXPR;

    bool isArithmetic =
        expr.kind == ASTKind::ADD_BINARYOP_EXPR || 
        expr.kind == ASTKind::SUB_BINARYOP_EXPR || 
        expr.kind == ASTKind::MUL_BINARYOP_EXPR || 
        expr.kind == ASTKind::DIV_BINARYOP_EXPR || 
        expr.kind == ASTKind::MOD_BINARYOP_EXPR;

    if (isLogical || isArithmetic) {
        Type lhs = VerifyExpression(expr.binaryOp.left, symbolTable);
        Type rhs = VerifyExpression(expr.binaryOp.right, symbolTable);
        bool lhsIsScalar = lhs.numPointerLevels == 0;
        bool rhsIsScalar = rhs.numPointerLevels == 0;
        bool lhsIsIntegral = IsIntegralType(lhs);
        bool rhsIsIntegral = IsIntegralType(rhs);
        bool ok = false;
        Type resultType;

        // Allowed binary operations:
        //   - Pointer arithmetic (ptr+int) (add/sub only)
        //   - Same primitive type
        //   - Any (pointer) comparison (T==T) (logical only)
        if (!lhsIsScalar && rhsIsScalar && rhsIsIntegral &&
            (expr.kind == ASTKind::ADD_BINARYOP_EXPR || expr.kind == ASTKind::SUB_BINARYOP_EXPR))
        {
            resultType = lhs;
        }

        if (lhsIsScalar && rhsIsScalar &&
            lhs.typeName == rhs.typeName &&
            IsPrimitiveType(lhs))
        {
            resultType = lhs;
        }

        if (isLogical && lhs == rhs) {
            resultType = { .typeName="u8", .numPointerLevels=0 };
        }

        if (resultType.typeName.empty()) {
            CompileErrorAt(token, "Invalid operands to binary operator ('{}{:@>{}}' {} '{}{:@>{}}')",
                lhs.typeName, "", lhs.numPointerLevels,
                token.text,
                rhs.typeName, "", rhs.numPointerLevels);
        }

        AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.op_kind=expr.kind, .accessKind=TypeAccessKind(resultType)}});
        return resultType;
    }

    assert(0 && "Unreachable");
    return {};
}

void Analyzer::VerifyStatement(ASTIndex stmtIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    const ASTNode& stmt = ast.tree[stmtIdx];
    switch (stmt.kind) {
        case ASTKind::IF_STATEMENT: {
            ++blockDepth;
            stackAddrs.push_back(stackAddrs.back());
            blockStackSize.push_back(blockStackSize.back());
            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            Type condType = VerifyExpression(stmt.ifstmt.cond, innerScope);
            if (!IsPrimitiveType(condType) && condType.numPointerLevels == 0) {
                CompileErrorAt(tokens[stmt.tokenIdx],
                    "Cannot evaluate non-primitive or non-scalar expression in if statement condition");
            }

            size_t ifJmpIdx = instructions.size();
            AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP_Z});
            size_t accessSize = TypeSize(condType);
            assert(accessSize <= 8);
            instructions[ifJmpIdx].jmp.accessSize = accessSize;
            VerifyStatements(stmt.ifstmt.body, innerScope);

            // If there is an else block, jump one more to skip the fi jump
            bool hasElse = stmt.ifstmt.elsestmt != AST_EMPTY;
            instructions[ifJmpIdx].jmp.jmpAddr = instructions.size() + hasElse;

            if (hasElse) {
                size_t fiJmpIdx = instructions.size();
                AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
                VerifyStatements(stmt.ifstmt.elsestmt, innerScope);
                instructions[fiJmpIdx].jmpAddr = instructions.size();
            }

            --blockDepth;
            stackAddrs.pop_back();
            blockStackSize.pop_back();
        } break;

        case ASTKind::FOR_STATEMENT: {
            ++blockDepth;
            ++loopDepth;
            stackAddrs.push_back(stackAddrs.back());
            blockStackSize.push_back(blockStackSize.back());
            breakAddrs.emplace_back();
            continueAddrs.emplace_back();

            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            if (stmt.forstmt.init != AST_NULL)
                VerifyDefinition(stmt.forstmt.init, innerScope);

            size_t cmpAddr = instructions.size();

            bool hasCondition = stmt.forstmt.cond != AST_NULL;
            Type condType;
            if (hasCondition) {
                condType = VerifyExpression(stmt.forstmt.cond, innerScope);
                if (!IsPrimitiveType(condType) && condType.numPointerLevels == 0) {
                    CompileErrorAt(tokens[stmt.tokenIdx],
                        "Cannot evaluate non-primitive or non-scalar expression in for statement condition");
                }
            }
            size_t whileJmpIdx = instructions.size();

            if (hasCondition) {
                AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP_Z});
                size_t accessSize = TypeSize(condType);
                assert(accessSize <= 8);
                instructions[whileJmpIdx].jmp.accessSize = accessSize;
            }

            VerifyStatements(stmt.forstmt.body, innerScope);


            for (size_t continueAddr : continueAddrs.back()) {
                instructions[continueAddr].jmpAddr = instructions.size();
            }

            if (stmt.forstmt.incr != AST_NULL)
                VerifyStatement(stmt.forstmt.incr, innerScope);

            AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP, .jmpAddr=cmpAddr});
            if (hasCondition) {
                instructions[whileJmpIdx].jmpAddr = instructions.size();
            }

            for (size_t breakAddr : breakAddrs.back()) {
                instructions[breakAddr].jmpAddr = instructions.size();
            }

            --blockDepth;
            --loopDepth;
            stackAddrs.pop_back();
            blockStackSize.pop_back();
            breakAddrs.pop_back();
            continueAddrs.pop_back();
        } break;

        case ASTKind::DEFINITION: {
            VerifyDefinition(stmtIdx, symbolTable);
        } break;

        case ASTKind::RETURN_STATEMENT: {
            // TODO: Stop generating instructions for current block after returning
            bool hasReturnValue = stmt.ret.expr != AST_NULL;
            if (hasReturnValue != currProc->hasReturnType) {
                if (currProc->hasReturnType)
                    CompileErrorAt(tokens[stmt.tokenIdx], "Procedure returns a value");
                else
                    CompileErrorAt(tokens[stmt.tokenIdx], "Procedure does not return a value");
            }
            if (hasReturnValue) {
                Type retType = VerifyExpression(stmt.ret.expr, symbolTable);
                AssertTypesAreSame(currProc->returnType, retType, tokens[stmt.tokenIdx]);
            }
            if (blockDepth == 0) {
                returnAtTopLevel = true;
            }

            AddInstruction(Instruction{.opcode=hasReturnValue ?
                Instruction::Opcode::RETURN_VAL :
                Instruction::Opcode::RETURN_VOID});
        } break;

        case ASTKind::BREAK_STATEMENT: {
            if (loopDepth <= 0) {
                CompileErrorAt(tokens[ast.tree[stmtIdx].tokenIdx],
                    "Break statement not within a loop");
            }
            breakAddrs.back().push_back(instructions.size());
            AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
        } break;

        case ASTKind::CONTINUE_STATEMENT: {
            if (loopDepth <= 0) {
                CompileErrorAt(tokens[ast.tree[stmtIdx].tokenIdx],
                    "Continue statement not within a loop");
            }
            continueAddrs.back().push_back(instructions.size());
            AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
        } break;

        case ASTKind::ASSIGN: {
            const ASTNode::ASTAssign& asgn = stmt.asgn;
            const Token& lhsTok = tokens[ast.tree[asgn.lvalue].tokenIdx];
            if (!symbolTable.contains(lhsTok.text)) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Assignment to undefined identifier {}", lhsTok.text);
            }
            const ASTNode::ASTDefinition& defn = ast.tree[symbolTable[lhsTok.text]].defn;
            if (defn.isConst) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Assignment of read-only variable {}", lhsTok.text);
            }

            Type lhs = VerifyType(defn.type);
            Type rhs = VerifyExpression(stmt.asgn.rvalue, symbolTable);
            AssertTypesAreSame(lhs, rhs, tokens[stmt.tokenIdx]);
            VerifyLValue(asgn.lvalue, symbolTable, false);
        } break;

        case ASTKind::ASM_STATEMENT: {
            for (ASTIndex i : ast.lists[stmt.asm_.strings]) {
                AddInstruction(Instruction{.opcode=Instruction::Opcode::INLINE, .str=ast.tree[i].literal.str});
            }
        } break;

        default: {
            // Treat everything else as expression
            VerifyExpression(stmtIdx, symbolTable);
        } break;
    }
}


void Analyzer::VerifyStatements(ASTList list, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    if (list != AST_EMPTY) {
        for (ASTIndex stmtIdx : ast.lists[list]) {
            VerifyStatement(stmtIdx, symbolTable);
        }
    }
}

void Analyzer::VerifyProcedure(ASTIndex procIdx) {
    const ASTNode proc = ast.tree[procIdx];
    const ASTNode::ASTProcedure& procedure = proc.procedure;
    const Token& token = tokens[proc.tokenIdx];

    ASTList params = procedure.params;
    std::unordered_map<std::string_view, ASTIndex> symbolTable; // TODO: use more efficient structure
    // When entering and leaving lexical scopes, the entire table needs to be copied
    // This is probably not a good idea, but I can't really think of anything better right now
    // Maybe some sort of linked structure (constant insert/remove)?

    ProcedureDefn& procDefn = procedureDefns.at(token.text);
    currProc = &procDefn;
    procDefn.instructionNum = instructions.size();

    returnAtTopLevel = false;
    maxStaticStackSize = 0;
    stackAddrs.emplace_back();
    blockStackSize.push_back(0);

    size_t enterIdx = instructions.size();
    AddInstruction(Instruction{.opcode=Instruction::Opcode::ENTER});

    // Don't allocate arrays passed as arguments
    VerifyStatements(params, symbolTable);

    if (!proc.procedure.isExtern) {
        VerifyStatements(procedure.body, symbolTable);
        if (!returnAtTopLevel) {
            if (procDefn.hasReturnType) {
                CompileErrorAt(token, "Non-void procedure '{}' did not return in all control paths", token.text);
            }
            AddInstruction(Instruction{.opcode=procDefn.hasReturnType ?
                Instruction::Opcode::RETURN_VAL :
                Instruction::Opcode::RETURN_VOID});
        }
        instructions[enterIdx].frame.localStackSize = maxStaticStackSize - 8*procDefn.paramTypes.size();
        instructions[enterIdx].frame.numParams = procDefn.paramTypes.size();
    }

    stackAddrs.pop_back();
    blockStackSize.pop_back();
}

class GraphUtil {
    const std::vector<std::vector<uint32_t>>& adj;
    const size_t N;
    std::vector<bool> vis;
    std::vector<bool> stk;
    std::vector<uint32_t> order;

    bool HasCycle(uint32_t u) {
        if (!vis[u]) {
            vis[u] = true;
            stk[u] = true;
            for (uint32_t v : adj[u]) {
                if (!vis[v])
                    if (HasCycle(v))
                        return true;
                if (stk[v])
                    return true;
            }
            stk[u] = false;
        }
        return false;
    }

    void TopologicalSort(uint32_t u) {
        vis[u] = true;
        for (uint32_t v : adj[u])
            if (!vis[v])
                TopologicalSort(v);
        order.push_back(u);
    }

public:
    GraphUtil(const std::vector<std::vector<uint32_t>>& adj_)
        : adj{adj_}, N{adj_.size()}, vis(adj_.size()), stk(adj_.size())
    {}

    uint32_t HasCycle() {
        std::fill(vis.begin(), vis.end(), false);
        std::fill(stk.begin(), stk.end(), false);
        for (uint32_t i = 0; i < N; ++i)
            if (HasCycle(i)) return i;
        return adj.size();
    }

    std::vector<uint32_t> TopologicalSort() {
        order.clear();
        order.reserve(N);
        std::fill(vis.begin(), vis.end(), false);
        for (uint32_t i = 0; i < N; ++i)
            if (!vis[i]) TopologicalSort(i);
        return order;
    }
};

void Analyzer::VerifyProgram() {

    const ASTNode& prog = ast.tree[0];

    // Resolve all type definitions
    const auto& structList = ast.lists[prog.program.structs];
    std::unordered_map<std::string_view, uint32_t> structIds;
    structIds["u8"]  = structList.size()+0; resolvedTypes["u8"]  = { .size=1, .alignment=1, .storageKind=TypeDefn::StorageKind::U8 };
    structIds["i64"] = structList.size()+1; resolvedTypes["i64"] = { .size=8, .alignment=8, .storageKind=TypeDefn::StorageKind::I64 };
    // structIds["f32"] = structList.size()+2; resolvedTypes["f32"] = { .size=4, .alignment=4, .storageKind=TypeDefn::StorageKind::F32 };
    structIds["f64"] = structList.size()+2; resolvedTypes["f64"] = { .size=8, .alignment=8, .storageKind=TypeDefn::StorageKind::F64 };

    for (size_t i = 0; i < structList.size(); ++i) {
        ASTIndex structIdx = structList[i];
        const ASTNode& st = ast.tree[structIdx];
        const Token& structName = tokens[st.tokenIdx];
        if (structIds.contains(structName.text))
            CompileErrorAt(structName,
                           "Redeclaration of type \"{}\"",
                           structName.text);
        structIds[structName.text] = i;
    }

    uint32_t N = structIds.size();
    std::vector<std::vector<uint32_t>> depAdjList(N);

    for (ASTIndex structIdx : structList) {
        const ASTNode& st = ast.tree[structIdx];
        const Token& structName = tokens[st.tokenIdx];

        ASTList members = st.struct_.members;
        uint32_t structId = structIds.at(structName.text);
        std::vector<uint32_t>& dependOn = depAdjList[structId];

        if (ast.lists[members].empty()) {
            CompileErrorAt(structName, "Struct cannot be empty");
        }

        for (ASTIndex memberIdx : ast.lists[members]) {
            const ASTNode& memberType = ast.tree[ast.tree[memberIdx].defn.type];
            const Token& memberName = tokens[memberType.tokenIdx];

            auto it = structIds.find(memberName.text);
            if (it == structIds.end()) {
                CompileErrorAt(memberName, "Unknown type \"{}\"", memberName.text);
            }
            uint32_t memberId = it->second;

            bool isPointer = memberType.type.numPointerLevels != 0;
            if (!isPointer) {
                dependOn.push_back(memberId);
            }
        }
    }

    // Detect cycle in graph
    GraphUtil graph{depAdjList};
    uint32_t cycleStartId = graph.HasCycle();

    if (cycleStartId != N) {
        ASTIndex structIdx = structList[cycleStartId];
        const ASTNode& st = ast.tree[structIdx];
        const Token& structName = tokens[st.tokenIdx];
        CompileErrorAt(structName, "Cyclic struct definition in \"{}\" detected!", structName.text);
    }

    // Resolve types
    std::vector<uint32_t> postorder = graph.TopologicalSort();
    for (uint32_t i : postorder) {
        // Skip builtins
        if (i >= structList.size()) continue;

        ASTIndex structIdx = structList[i];
        const ASTNode& st = ast.tree[structIdx];
        const Token& structName = tokens[st.tokenIdx];
        // fmt::print(stderr, "{}\n", structName.text);

        ASTList members = st.struct_.members;

        TypeDefn& type = resolvedTypes[structName.text];
        type.storageKind = TypeDefn::StorageKind::STRUCT;
        for (ASTIndex memberIdx : ast.lists[members]) {
            const ASTNode& memberType = ast.tree[ast.tree[memberIdx].defn.type];
            const Token& memberName = tokens[memberType.tokenIdx];

            bool isPointer = memberType.type.numPointerLevels != 0;
            size_t memberSize;
            size_t memberAlignment;
            if (isPointer) {
                memberSize = 8;
                memberAlignment = 8;
            }
            else {
                // fmt::print(stderr, "    {}\n", memberName.text);
                auto it = resolvedTypes.find(memberName.text);
                assert(it != resolvedTypes.end());

                memberSize = it->second.size;
                memberAlignment = it->second.alignment;
            }

            // Round up to align
            type.size = RoudUpToPowerOfTwo(type.size + memberSize, memberAlignment);

            type.memberOffsets.emplace_back(Type{
                .typeName = memberName.text,
                .numPointerLevels = memberType.type.numPointerLevels,
            }, type.size - memberSize);

            if (type.alignment < memberAlignment)
                type.alignment = memberAlignment;
        }

        type.size = RoudUpToPowerOfTwo(type.size, type.alignment);

        // for (auto& [k,v] : type.memberOffsets) {
        //     fmt::print(stderr, "  {}{:@>{}} - {}\n",
        //                k.typeName,
        //                "", k.numPointerLevels,
        //                v);
        // }
        // fmt::print(stderr, "size={}\n", type.size);
        // fmt::print(stderr, "align={}\n", type.alignment);
    }

    // Builtins (but not really, just regular predefined procedures)
    procedureDefns["sqrt"] = ProcedureDefn{ .paramTypes = { Type{"f64", 0} }, .returnType = Type{"f64", 0}, .hasReturnType = true, .instructionNum = BUILTIN_sqrt };
    // procedureDefns["puti"]
    // procedureDefns["putf"]
    // procedureDefns["puts"]
    // procedureDefns["putcs"]
    // procedureDefns["putc"]

    // Collect all procedure definitions and "forward declare" them.
    // Mutual recursion should work out of the box
    const auto& procList = ast.lists[prog.program.procedures];
    for (ASTIndex procIdx : procList) {
        const ASTNode& proc = ast.tree[procIdx];
        const Token& procName = tokens[proc.tokenIdx];
        if (procedureDefns.contains(procName.text)) {
            CompileErrorAt(procName, "Redefinition of procedure '{}'", procName.text);
        }

        const std::vector<ASTIndex>& params = ast.lists[proc.procedure.params];
        std::vector<Type> paramTypes;
        paramTypes.reserve(params.size());
        for (ASTIndex paramIdx : params)
            paramTypes.push_back(VerifyType(ast.tree[paramIdx].defn.type));
        bool hasReturnType = proc.procedure.retType_ != AST_NULL;
        procedureDefns[procName.text] = hasReturnType ?
            ProcedureDefn{ .paramTypes = std::move(paramTypes), .returnType = VerifyType(proc.procedure.retType_), .hasReturnType = true } :
            ProcedureDefn{ .paramTypes = std::move(paramTypes), .hasReturnType = false };
    }

    // Constructors - procedures with the same name as the struct, and same parameters as members
    for (ASTIndex structIdx : structList) {
        const ASTNode& st = ast.tree[structIdx];
        const Token& structName = tokens[st.tokenIdx];
        if (procedureDefns.contains(structName.text)) {
            CompileErrorAt(structName, "Definition of struct with same name as procedure '{}'", structName.text);
        }

        const std::vector<ASTIndex>& members = ast.lists[st.struct_.members];
        std::vector<Type> paramTypes;
        paramTypes.reserve(members.size());
        for (ASTIndex paramIdx : members)
            paramTypes.push_back(VerifyType(ast.tree[paramIdx].defn.type));
        procedureDefns[structName.text] = ProcedureDefn{
                .paramTypes = std::move(paramTypes),
                .returnType = Type{.typeName=structName.text, .numPointerLevels=0},
                .hasReturnType = true
            };
    }

    for (ASTIndex procIdx : procList) {
        const ASTNode& proc = ast.tree[procIdx];
        const Token& procName = tokens[proc.tokenIdx];
        procedures.emplace_back();
        procedures.back().procInfo = proc.procedure;
        procedures.back().procName = procName.text;
        procedures.back().insStartIdx = instructions.size();
        if (proc.procedure.isExtern) {
            keepGenerating = false;
        }
        VerifyProcedure(procIdx);
        if (proc.procedure.isExtern) {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::CALL, .str={procName.text.data(), procName.text.size()}});
            keepGenerating = true;
        }
        procedures.back().insEndIdx = instructions.size();
        for (ASTIndex parameterIdx : ast.lists[proc.procedure.params]) {
            const ASTNode& param = ast.tree[parameterIdx];
            procedures.back().params.push_back(param.defn);
        }
    }

    for (auto [jumpIdx, procName] : unresolvedCalls) {
        instructions.at(jumpIdx).jmpAddr = procedureDefns.at(procName).instructionNum;
    }

    for (auto& proc : procedures) {
        for (size_t i = proc.insStartIdx; i < proc.insEndIdx; ++i)
            proc.instructions.push_back(instructions[i]);
    }
}

std::vector<Procedure> VerifyAST(const std::vector<Token>& tokens, AST& ast) {
    Analyzer analyzer{tokens, ast};
    analyzer.VerifyProgram();
    return std::move(analyzer.procedures);
}
