#include "analyzer.hpp"
#include "compileerror.hpp"
#include "parser.hpp"

#include <cassert>

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

    bool isScalar = defn.arraySize == AST_NULL;
    size_t offset = symbolTable.size();

    if (!isScalar) {
        Type arrSize = VerifyExpression(defn.arraySize, symbolTable);
        assert(arrSize.isScalar);
        if (arrSize.kind != TypeKind::I64) {
            CompileErrorAt(asgnToken, "Size of array '{}' has non-integer type (got {})",
                ident, TypeKindName(arrSize.kind));
        }

        // Scale by the width of the array type (replace with a left shift when implemented)
        if (defn.type == TypeKind::I64 || defn.type == TypeKind::F64) {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::I64,.i64=8}});
            AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.kind=TypeKind::I64,.op_kind=ASTKind::MUL_BINARYOP_EXPR}});
        }

        AddInstruction(Instruction{.opcode=Instruction::Opcode::ALLOCA, .varAddr=offset});
    }

    if (defn.initExpr != AST_NULL) {
        Type rhsType = VerifyExpression(defn.initExpr, symbolTable);
        assert(rhsType.isScalar);
        if (!isScalar || defn.type != rhsType.kind) {
            CompileErrorAt(asgnToken, "Incompatible types when initializing '{}' to type {} (expected {})",
                ident, TypeKindName(rhsType.kind), TypeKindName(defn.type));
        }
        AddInstruction(Instruction{.opcode=Instruction::Opcode::STORE_FAST, .varAddr=offset});
    }

    AssertIdentUnusedInCurrentScope(symbolTable, token);

    stackAddrs.back()[ident] = offset;
    symbolTable[ident] = defnIdx;

    if (maxNumVariables < offset + 1)
        maxNumVariables = offset + 1;
}

Type Analyzer::VerifyLValue(ASTIndex lvalIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable, bool isLoading) {
    const ASTNode& expr = ast.tree[lvalIdx];
    const Token& token = tokens[expr.tokenIdx];

    if (!symbolTable.contains(token.text)) {
        CompileErrorAt(token, "Use of undefined identifier '{}'", token.text);
    }
    const ASTNode::ASTDefinition& defn = ast.tree[symbolTable[token.text]].defn;
    bool isScalar = defn.arraySize == AST_NULL;
    bool hasSubscript = expr.lvalue.subscript != AST_NULL;
    // FIXME: Non-scalar expressions are entirely disallowed
    // Maybe change this when strings need to work correctly
    if (isScalar) {
        if (hasSubscript) {
            CompileErrorAt(token, "Cannot subscript scalar variable '{}'", token.text);
        }

        if (isLoading) {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD_FAST, .varAddr=stackAddrs.back().at(token.text)});
        }
        else {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::STORE_FAST, .varAddr=stackAddrs.back().at(token.text)});
        }
    }
    else {
        if (!hasSubscript) {
            CompileErrorAt(token, "Subscript required for non-scalar variable '{}'", token.text);
        }
        Type subType = VerifyExpression(expr.lvalue.subscript, symbolTable);
        if (!subType.isScalar || subType.kind != TypeKind::I64) {
            CompileErrorAt(tokens[ast.tree[expr.lvalue.subscript].tokenIdx],
                "Cannot subscript with non-scalar integral variable '{}'",
                token.text);
        }

        // Scale by the width of the array type (replace with a left shift when implemented)
        if (defn.type == TypeKind::I64 || defn.type == TypeKind::F64) {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::I64,.i64=8}});
            AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.kind=TypeKind::I64,.op_kind=ASTKind::MUL_BINARYOP_EXPR}});
        }

        // Push (i * w + a) where a is the array (pointer) and i is the subscript
        AddInstruction(Instruction{.opcode=Instruction::Opcode::LOAD_FAST, .varAddr=stackAddrs.back().at(token.text)});
        AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.kind=TypeKind::I64,.op_kind=ASTKind::ADD_BINARYOP_EXPR}});

        if (isLoading) {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::DEREF});
        }
        else {
            AddInstruction(Instruction{.opcode=Instruction::Opcode::STORE});
        }
    }

    return { defn.type, true };
}


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
    const auto& args = ast.lists[call.args];
    size_t numArgs = args.size();


    // Push the address of the instruction following the jump
    size_t pushAddrIdx = instructions.size();
    AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::I64}});
    AddInstruction(Instruction{.opcode=Instruction::Opcode::SAVE});

    if (!procedureDefns.contains(callTok.text)) {
        CompileErrorAt(callTok, "Call to undefined procedure '{}'", callTok.text);
    }

    const ProcedureDefn& defn = procedureDefns.at(callTok.text);
    size_t numParams = defn.paramTypes.size();
    if (numArgs != numParams) {
        if (numParams == 1) {
            CompileErrorAt(callTok, "Expected {} argument, got {} instead", numParams, numArgs);
        }
        else {
            CompileErrorAt(callTok, "Expected {} arguments, got {} instead", numParams, numArgs);
        }
    }

    for (size_t argIdx{}; argIdx < numArgs; ++argIdx) {
        const ASTNode::ASTDefinition& param = ast.tree[defn.paramTypes[argIdx]].defn;
        Type argType = VerifyExpression(args[argIdx], symbolTable);
        if (param.type != argType.kind) {
            const Token& argTok = tokens[ast.tree[args[argIdx]].tokenIdx];
            CompileErrorAt(argTok, "Incompatible type {} for argument {} of '{}' (expected {})",
                TypeKindName(argType.kind), argIdx+1, callTok.text, TypeKindName(param.type));
        }
        // NOTE: Don't check sizes of arrays (decay to pointer)
    }

    // Defer resolving this address until all procedures have been generated
    unresolvedCalls.emplace_back(instructions.size(), callTok.text);
    AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
    instructions[pushAddrIdx].lit.i64 = instructions.size();

    // NOTE: The parser does not allow arrays as return types
    return { defn.returnType, true };
}

Type Analyzer::VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    assert(exprIdx != AST_NULL);
    const ASTNode& expr = ast.tree[exprIdx];
    switch (expr.kind) {
        case ASTKind::LVALUE_EXPR:
            return VerifyLValue(exprIdx, symbolTable, true);
        case ASTKind::INTEGER_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::I64,.i64=expr.literal.i64}});
            return { TypeKind::I64, true };
        case ASTKind::FLOAT_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::F64,.f64=expr.literal.f64}});
            return { TypeKind::F64, true };
        case ASTKind::CHAR_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::U8,.u8=expr.literal.u8}});
            return { TypeKind::U8, true };
        case ASTKind::STRING_LITERAL_EXPR:
            AddInstruction(Instruction{.opcode=Instruction::Opcode::PUSH, .lit={.kind=TypeKind::STR,.str=expr.literal.str}});
            return { TypeKind::STR, true };
        case ASTKind::CALL_EXPR: {
            return VerifyCall(exprIdx, symbolTable);
        }
        default: break;
    }

    // Otherwise, expect a binary or unary operator
    const Token& token = tokens[expr.tokenIdx];
    if (expr.kind == ASTKind::NEG_UNARYOP_EXPR ||
        expr.kind == ASTKind::NOT_UNARYOP_EXPR)
    {
        Type exprType = VerifyExpression(expr.unaryOp.expr, symbolTable);
        if (!exprType.isScalar) {
            CompileErrorAt(token, "Could not apply unary operator to non-scalar type");
        }
        if (expr.kind == ASTKind::NEG_UNARYOP_EXPR) {
            if (!(exprType.kind == TypeKind::I64 || exprType.kind == TypeKind::U8 || exprType.kind == TypeKind::F64)) {
                CompileErrorAt(token, "Cannot negate non-numeric type {}", TypeKindName(exprType.kind));
            }
        }
        else {
            if (!(exprType.kind == TypeKind::I64 || exprType.kind == TypeKind::U8)) {
                CompileErrorAt(token, "Cannot complement of non-integral type {}", TypeKindName(exprType.kind));
            }
        }

        AddInstruction(Instruction{.opcode=Instruction::Opcode::UNARY_OP, .op={.kind=exprType.kind,.op_kind=expr.kind}});
        return exprType;
    }

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
        Type lhsExprType = VerifyExpression(expr.binaryOp.left, symbolTable);
        Type rhsExprType = VerifyExpression(expr.binaryOp.right, symbolTable);
        if (!lhsExprType.isScalar || !rhsExprType.isScalar) {
            assert(0 && "Unreachable");
            // CompileErrorAt(token, "Could not apply binary operator {} to non scalar type '{}[]'",
            //     token.text,
            //     TypeKindName(lhsExprType.isScalar ? rhsExprType.kind : lhsExprType.kind));
        }
        if (lhsExprType.kind != rhsExprType.kind) {
            CompileErrorAt(token, "Invalid operands to binary operator ('{}' {} '{}')",
                TypeKindName(lhsExprType.kind), token.text, TypeKindName(rhsExprType.kind));
        }

        if (isLogical) {
            if (!(lhsExprType.kind == TypeKind::U8 || lhsExprType.kind == TypeKind::I64 || lhsExprType.kind == TypeKind::F64)) {
                CompileErrorAt(token, "Invalid operands to binary operator ('{}' {} '{}')",
                    TypeKindName(lhsExprType.kind), token.text, TypeKindName(rhsExprType.kind));
            }

            AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.kind=lhsExprType.kind,.op_kind=expr.kind}});
            return { TypeKind::U8, true };
        }
        else {
            // TODO: Disallow invalid binary operations (ex. str + int)

            AddInstruction(Instruction{.opcode=Instruction::Opcode::BINARY_OP, .op={.kind=lhsExprType.kind,.op_kind=expr.kind}});
            return lhsExprType;
        }
    }
    
    assert(0 && "Unreachable");
    return { TypeKind::NONE, 0 };
}

void Analyzer::VerifyStatement(ASTIndex stmtIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    const ASTNode& stmt = ast.tree[stmtIdx];
    switch (stmt.kind) {
        case ASTKind::IF_STATEMENT: {
            ++blockDepth;
            stackAddrs.push_back(stackAddrs.back());
            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            VerifyExpression(stmt.ifstmt.cond, innerScope);
            
            size_t ifJmpIdx = instructions.size();
            AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP_Z});
            VerifyStatements(stmt.ifstmt.body, innerScope);

            // If there is an else block, jump one more to skip the fi jump
            bool hasElse = stmt.ifstmt.elsestmt != AST_EMPTY;
            instructions[ifJmpIdx].jmpAddr = instructions.size() + hasElse;

            if (hasElse) {
                size_t fiJmpIdx = instructions.size();
                AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
                VerifyStatements(stmt.ifstmt.elsestmt, innerScope);
                instructions[fiJmpIdx].jmpAddr = instructions.size();
            }

            --blockDepth;
            stackAddrs.pop_back();
        } break;

        case ASTKind::FOR_STATEMENT: {
            ++blockDepth;
            ++loopDepth;
            stackAddrs.push_back(stackAddrs.back());
            breakAddrs.emplace_back();
            continueAddrs.emplace_back();

            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            if (stmt.forstmt.init != AST_NULL)
                VerifyDefinition(stmt.forstmt.init, innerScope);

            size_t cmpAddr = instructions.size();

            bool hasCondition = stmt.forstmt.cond != AST_NULL;
            if (hasCondition) {
                VerifyExpression(stmt.forstmt.cond, innerScope);
            }
            size_t whileJmpIdx = instructions.size();

            if (hasCondition) {
                AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP_Z});
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
            breakAddrs.pop_back();
            continueAddrs.pop_back();
        } break;

        case ASTKind::DEFINITION: {
            VerifyDefinition(stmtIdx, symbolTable);
        } break;

        case ASTKind::RETURN_STATEMENT: {
            // TODO: Stop generating instructions for current block after returning
            bool hasReturnValue = stmt.ret.expr != AST_NULL;
            Type retType = hasReturnValue ?
                VerifyExpression(stmt.ret.expr, symbolTable) :
                Type{ TypeKind::NONE, true };
            if (!retType.isScalar || retType.kind != currProc->returnType) {
                CompileErrorAt(tokens[ast.tree[stmtIdx].tokenIdx],
                    "Incompatible return type {}, expected {}",
                    TypeKindName(retType.kind), TypeKindName(currProc->returnType));
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

            bool isScalar = defn.arraySize == AST_NULL;
            bool hasSubscript = ast.tree[stmt.asgn.lvalue].lvalue.subscript != AST_NULL;
            Type rhsType = VerifyExpression(stmt.asgn.rvalue, symbolTable);
            assert(rhsType.isScalar);
            if (defn.type != rhsType.kind) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Incompatible types when assigning '{}' to type '{}' (expected '{}')",
                    lhsTok.text, TypeKindName(rhsType.kind), TypeKindName(defn.type));
            }
            if (!isScalar && !hasSubscript) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Subscript required for non-scalar variable '{}'", lhsTok.text);
            }
            else if (isScalar && hasSubscript) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Cannot subscript scalar variable '{}'", lhsTok.text);
            }

            VerifyLValue(asgn.lvalue, symbolTable, false);
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

    if (!hasEntry && token.text == "entry") {
        hasEntry = true;
        entryAddr = instructions.size();
        // TODO: Check arguments and return type of main
    }

    returnAtTopLevel = false;
    maxNumVariables = 0;
    stackAddrs.emplace_back();

    size_t enterIdx = instructions.size();
    AddInstruction(Instruction{.opcode=Instruction::Opcode::ENTER});

    // Don't allocate arrays passed as arguments
    keepGenerating = false;
    VerifyStatements(params, symbolTable);
    keepGenerating = true;

    VerifyStatements(procedure.body, symbolTable);
    if (!returnAtTopLevel) {
        if (procedure.retType != TypeKind::NONE) {
            CompileErrorAt(token, "Non-void procedure '{}' did not return in all control paths", token.text);
        }
        bool hasReturnValue = procDefn.returnType != TypeKind::NONE;
        AddInstruction(Instruction{.opcode=hasReturnValue ?
            Instruction::Opcode::RETURN_VAL :
            Instruction::Opcode::RETURN_VOID});
    }

    procDefn.stackSpace = maxNumVariables - procDefn.paramTypes.size();
    instructions[enterIdx].frame.numLocals = procDefn.stackSpace;
    instructions[enterIdx].frame.numParams = procDefn.paramTypes.size();

    stackAddrs.pop_back();
}

void Analyzer::VerifyProgram() {

    // Builtins
    ast.tree.push_back({ .defn = { .type = TypeKind::F64 }, .kind = ASTKind::DEFINITION, });
    ASTIndex f64Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::I64 }, .kind = ASTKind::DEFINITION, });
    ASTIndex i64Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::U8 }, .kind = ASTKind::DEFINITION, });
    ASTIndex u8Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::STR }, .kind = ASTKind::DEFINITION, });
    ASTIndex strParam = ast.tree.size() - 1;

    procedureDefns["sqrt"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::F64,  .instructionNum=BUILTIN_sqrt };
    procedureDefns["puti"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::NONE, .instructionNum=BUILTIN_puti };
    procedureDefns["putf"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::NONE, .instructionNum=BUILTIN_putf };
    procedureDefns["puts"] = ProcedureDefn{ .paramTypes = { strParam }, .returnType = TypeKind::NONE, .instructionNum=BUILTIN_puts };
    procedureDefns["itof"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::F64,  .instructionNum=BUILTIN_itof };
    procedureDefns["ftoi"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::I64,  .instructionNum=BUILTIN_ftoi };
    procedureDefns["itoc"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::U8,   .instructionNum=BUILTIN_itoc };
    procedureDefns["ctoi"] = ProcedureDefn{ .paramTypes = { u8Param },  .returnType = TypeKind::I64,  .instructionNum=BUILTIN_ctoi };

    // Collect all procedure definitions and "forward declare" them.
    // Mutual recursion should work out of the box
    const ASTNode& prog = ast.tree[0];
    const auto& procedures = ast.lists[prog.program.procedures];
    for (ASTIndex procIdx : procedures) {
        const ASTNode& proc = ast.tree[procIdx];
        const Token& procName = tokens[proc.tokenIdx];
        if (procedureDefns.contains(procName.text)) {
            CompileErrorAt(procName, "Redefinition of procedure '{}'", procName.text);
        }

        const auto& params = ast.lists[proc.procedure.params];
        procedureDefns[procName.text] = ProcedureDefn{
                .paramTypes = params,
                .returnType = proc.procedure.retType,
            };
    }


    size_t entryJmpIdx = instructions.size();
    AddInstruction(Instruction{.opcode=Instruction::Opcode::JMP});
    for (ASTIndex procIdx : procedures) {
        VerifyProcedure(procIdx);
    }

    for (auto [jumpIdx, procName] : unresolvedCalls) {
        instructions.at(jumpIdx).jmpAddr = procedureDefns.at(procName).instructionNum;
    }

    if (!hasEntry) {
        CompileErrorAt(tokens.back(), "Missing entrypoint procedure 'entry'");
    }
    instructions[entryJmpIdx].jmpAddr = entryAddr;
}

void VerifyAST(File file, const std::vector<Token>& tokens, AST& ast) {
    Analyzer analyzer{file, tokens, ast};
    analyzer.VerifyProgram();
}
