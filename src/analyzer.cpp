#include "analyzer.hpp"
#include "compileerror.hpp"
#include "parser.hpp"

void Analyzer::AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident) {
    if (procedureDefns.contains(ident.text)) {
        // Variable name same as function name, not allowed
        CompileErrorAt(ident, "Local variable '{}' shadows function", ident.text);
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
    if (defn.initExpr != AST_NULL) {
        Type rhsType = VerifyExpression(defn.initExpr, symbolTable);
        assert(rhsType.isScalar);
        if (defn.type != rhsType.kind) {
            CompileErrorAt(tokens[ast.tree[defnIdx].tokenIdx + 1], "Incompatible types when initializing '{}' to type {} (expected {})",
                ident, TypeKindName(rhsType.kind), TypeKindName(defn.type));
        }
    }
    AssertIdentUnusedInCurrentScope(symbolTable, token);
    symbolTable[ident] = defnIdx;
}

Type Analyzer::VerifyCall(ASTIndex callIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    const Token& callTok = tokens[ast.tree[callIdx].tokenIdx];
    const ASTNode::ASTCall& call = ast.tree[callIdx].call;
    const auto& args = ast.lists[call.args];
    size_t numArgs = args.size();

    if (!procedureDefns.contains(callTok.text)) {
        CompileErrorAt(callTok, "Call to undefined function '{}'", callTok.text);
    }

    const ProcedureDefn& defn = procedureDefns[callTok.text];
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

    // NOTE: The parser does not allow arrays as return types
    return { defn.returnType, true };
}

Type Analyzer::VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable) {
    assert(exprIdx != AST_NULL);
    const ASTNode& expr = ast.tree[exprIdx];
    switch (expr.kind) {
        case ASTKind::INTEGER_LITERAL_EXPR: return { TypeKind::I64, true };
        case ASTKind::FLOAT_LITERAL_EXPR:   return { TypeKind::F64, true };
        case ASTKind::CHAR_LITERAL_EXPR:    return { TypeKind::U8, true };
        case ASTKind::STRING_LITERAL_EXPR:  return { TypeKind::STR, true };
        case ASTKind::CALL_EXPR: return VerifyCall(exprIdx, symbolTable);
        default: break;
    }

    const Token& token = tokens[expr.tokenIdx];
    if (expr.kind == ASTKind::LVALUE_EXPR) {
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
        }

        return { defn.type, true };
    }

    // Otherwise, expect a binary or unary operator
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

            return { TypeKind::U8, true };
        }
        else {
            // TODO: Disallow invalid binary operations (ex. str + int)

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
            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            VerifyStatements(stmt.ifstmt.body, innerScope);
            VerifyExpression(stmt.ifstmt.cond, innerScope);
            VerifyStatements(stmt.ifstmt.elsestmt, innerScope);
            --blockDepth;
        } break;

        case ASTKind::FOR_STATEMENT: {
            ++blockDepth;
            ++loopDepth;
            std::unordered_map<std::string_view, ASTIndex> innerScope = symbolTable;
            if (stmt.forstmt.init != AST_NULL)
                VerifyDefinition(stmt.forstmt.init, innerScope);
            if (stmt.forstmt.cond != AST_NULL)
                VerifyExpression(stmt.forstmt.cond, innerScope);
            if (stmt.forstmt.incr != AST_NULL)
                VerifyStatement(stmt.forstmt.incr, innerScope);
            VerifyStatements(stmt.forstmt.body, innerScope);
            --loopDepth;
            --blockDepth;
        } break;

        case ASTKind::DEFINITION: {
            VerifyDefinition(stmtIdx, symbolTable);
        } break;

        case ASTKind::RETURN_STATEMENT: {
            Type retType = stmt.ret.expr == AST_NULL ?
                Type{ TypeKind::NONE, true } :
                VerifyExpression(stmt.ret.expr, symbolTable);
            if (!retType.isScalar || retType.kind != currProcRetType) {
                CompileErrorAt(tokens[ast.tree[stmt.ret.expr].tokenIdx],
                    "Incompatible return type {}, expected {}",
                    TypeKindName(retType.kind), TypeKindName(currProcRetType));
            }
            if (blockDepth == 0) {
                returnAtTopLevel = true;
            }
        } break;

        case ASTKind::BREAK_STATEMENT: {
            if (loopDepth <= 0) {
                CompileErrorAt(tokens[ast.tree[stmtIdx].tokenIdx],
                    "Break statement not within a loop");
            }
        } break;

        case ASTKind::CONTINUE_STATEMENT: {
            if (loopDepth <= 0) {
                CompileErrorAt(tokens[ast.tree[stmtIdx].tokenIdx],
                    "Continue statement not within a loop");
            }
        } break;

        case ASTKind::ASSIGN: {
            const ASTNode::ASTAssign asgn = stmt.asgn;
            const Token& lhsTok = tokens[ast.tree[asgn.lvalue].tokenIdx];
            if (!symbolTable.contains(lhsTok.text)) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Assignment to undefined identifier {}", lhsTok.text);
            }
            const ASTNode::ASTDefinition& defn = ast.tree[symbolTable[lhsTok.text]].defn;
            if (defn.isConst) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Assignment of read-only variable {}", lhsTok.text);
            }
            VerifyExpression(stmt.asgn.rvalue, symbolTable);

            Type rhsType = VerifyExpression(stmt.asgn.rvalue, symbolTable);
            assert(rhsType.isScalar);
            if (defn.type != rhsType.kind) {
                CompileErrorAt(tokens[stmt.tokenIdx], "Incompatible types when assigning '{}' to type '{}' (expected '{}')",
                    lhsTok.text, TypeKindName(rhsType.kind), TypeKindName(defn.type));
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
    const ASTNode::ASTProcedure& procedure = ast.tree[procIdx].procedure;

    ASTList params = procedure.params;
    std::unordered_map<std::string_view, ASTIndex> symbolTable; // TODO: use more efficient structure
    // When entering and leaving lexical scopes, the entire table needs to be copied
    // This is probably not a good idea, but I can't really think of anything better right now
    // Maybe some sort of linked structure (constant insert/remove)?

    currProcRetType = procedure.retType;
    returnAtTopLevel = false;
    VerifyStatements(params, symbolTable);
    VerifyStatements(procedure.body, symbolTable);
    if (currProcRetType != TypeKind::NONE && !returnAtTopLevel) {
        CompileErrorAt(tokens[ast.tree[procIdx].tokenIdx], "Non-void function did not return in all control paths");
    }
}

void Analyzer::VerifyProgram() {
    const ASTNode& prog = ast.tree[0];

    // Builtin functions
    ast.tree.push_back({ .defn = { .type = TypeKind::F64 }, .kind = ASTKind::DEFINITION, });
    ASTIndex f64Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::I64 }, .kind = ASTKind::DEFINITION, });
    ASTIndex i64Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::U8 }, .kind = ASTKind::DEFINITION, });
    ASTIndex u8Param = ast.tree.size() - 1;
    ast.tree.push_back({ .defn = { .type = TypeKind::STR }, .kind = ASTKind::DEFINITION, });
    ASTIndex strParam = ast.tree.size() - 1;

    procedureDefns["sqrt"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::F64, };
    procedureDefns["puti"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::NONE, };
    procedureDefns["putf"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::NONE, };
    procedureDefns["puts"] = ProcedureDefn{ .paramTypes = { strParam }, .returnType = TypeKind::NONE, };
    procedureDefns["itof"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::F64, };
    procedureDefns["ftoi"] = ProcedureDefn{ .paramTypes = { f64Param }, .returnType = TypeKind::I64, };
    procedureDefns["itoc"] = ProcedureDefn{ .paramTypes = { i64Param }, .returnType = TypeKind::U8, };
    procedureDefns["ctoi"] = ProcedureDefn{ .paramTypes = { u8Param }, .returnType = TypeKind::I64, };

    bool hasEntry = false;
    // Collect all procedure definitions and "forward declare" them.
    // Mutual recursion should work out of the box
    const auto& procedures = ast.lists[prog.program.procedures];
    for (ASTIndex procIdx : procedures) {
        const ASTNode& proc = ast.tree[procIdx];
        const Token& procName = tokens[proc.tokenIdx];
        if (procedureDefns.contains(procName.text)) {
            CompileErrorAt(procName, "Redefinition of procedure '{}'", procName.text);
        }
        if (!hasEntry && procName.text == "entry") {
            hasEntry = true;
            // TODO: Check arguments and return type of main
        }

        const auto& params = ast.lists[proc.procedure.params];
        procedureDefns[procName.text] = ProcedureDefn{
                .paramTypes = params,
                .returnType = proc.procedure.retType,
            };
    }

    if (!hasEntry) {
        CompileErrorAt(tokens.back(), "Missing entrypoint procedure 'entry'");
    }

    for (ASTIndex procIdx : procedures) {
        VerifyProcedure(procIdx);
    }
}

void VerifyAST(File file, const std::vector<Token>& tokens, AST& ast) {
    Analyzer analyzer{file, tokens, ast};
    analyzer.VerifyProgram();
}
