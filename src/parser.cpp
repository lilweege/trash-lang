#include "parser.hpp"
#include "tokenizer.hpp"
#include "compileerror.hpp"

#include <array>
#include <cassert>

#include <charconv>
static bool ParseLiteral(std::string_view text, auto& out) {
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    auto [ptr, err] = std::from_chars(begin, end, out);
    bool ok = err == std::errc{} && ptr == end;
    return ok;
}

const char* ASTKindName(ASTKind kind) {
    static_assert(static_cast<uint32_t>(ASTKind::COUNT) == 31, "Exhaustive check of AST kinds failed");
    const std::array<const char*, static_cast<uint32_t>(ASTKind::COUNT)> ASTKindNames{
        "UNINITIALIZED",
        "PROGRAM",
        "PROCEDURE",
        "IF_STATEMENT",
        "FOR_STATEMENT",
        "RETURN_STATEMENT",
        "CONTINUE_STATEMENT",
        "BREAK_STATEMENT",
        "DEFINITION",
        "ASSIGN",
        "NEG_UNARYOP_EXPR",
        "NOT_UNARYOP_EXPR",
        "EQ_BINARYOP_EXPR",
        "NE_BINARYOP_EXPR",
        "GE_BINARYOP_EXPR",
        "GT_BINARYOP_EXPR",
        "LE_BINARYOP_EXPR",
        "LT_BINARYOP_EXPR",
        "AND_BINARYOP_EXPR",
        "OR_BINARYOP_EXPR",
        "ADD_BINARYOP_EXPR",
        "SUB_BINARYOP_EXPR",
        "MUL_BINARYOP_EXPR",
        "DIV_BINARYOP_EXPR",
        "MOD_BINARYOP_EXPR",
        "INTEGER_LITERAL_EXPR",
        "FLOAT_LITERAL_EXPR",
        "CHAR_LITERAL_EXPR",
        "STRING_LITERAL_EXPR",
        "LVALUE_EXPR",
        "CALL_EXPR",
    };
    return ASTKindNames[static_cast<uint32_t>(kind)];
}

const char* TypeKindName(TypeKind kind) {
    static_assert(static_cast<uint32_t>(TypeKind::COUNT) == 4, "Exhaustive check of type kinds failed");
    const std::array<const char*, static_cast<uint32_t>(TypeKind::COUNT)> TypeKindNames{
        "none",
        "u8",
        "i64",
        "f64",
    };
    return TypeKindNames[static_cast<uint32_t>(kind)];
}

#define CompileErrorAt(token, format, ...) CompileErrorAtToken(file, token, format, __VA_ARGS__)
#define ExpectAndConsumeToken(expectedKind, format, ...) do { \
        const Token& tok_ = PollCurrentToken(); \
        if (tok_.kind != expectedKind) \
            CompileErrorAt(tok_, format __VA_OPT__(,) __VA_ARGS__); \
    } while (0)


ASTIndex Parser::NewNode(ASTKind kind) {
    ast.tree.emplace_back();
    ast.tree.back().kind = kind;
    return static_cast<ASTIndex>(ast.tree.size()-1);
}

ASTIndex Parser::NewNodeFromToken(TokenIndex tokIdx, ASTKind kind) {
    ASTIndex idx = NewNode(kind);
    ASTNode& node = ast.tree[idx];
    node.tokenIdx = tokIdx;
    return idx;
}

ASTIndex Parser::NewNodeFromLastToken(ASTKind kind) {
    return NewNodeFromToken(tokenIdx - 1, kind);
}

const Token& Parser::PeekCurrentToken() const {
    assert(tokenIdx < tokens.size());
    return tokens[tokenIdx];
}

const Token& Parser::PollCurrentToken() {
    assert(tokenIdx < tokens.size());
    return tokens[tokenIdx++];
}

ASTList Parser::NewASTList() {
    ast.lists.emplace_back();
    return static_cast<ASTList>(ast.lists.size() - 1);
}

void Parser::AddToASTList(ASTList list, ASTIndex idx) {
    ast.lists[list].push_back(idx);
}

static void PrintIndent(uint32_t depth) {
    for (uint32_t i = 0; i < depth; ++i)
        fmt::print(stderr, "    ");
}

void Parser::PrintNode(ASTIndex rootIdx) const {
    const ASTNode& root = ast.tree[rootIdx];
    fmt::print(stderr, "{}: ", ASTKindName(root.kind));
    assert(root.tokenIdx != TOKEN_NULL);
    const Token& token = tokens[root.tokenIdx];
    fmt::print(stderr, "{}", token);
}

// TODO: make freestanding or make templated fmt::formatter
void Parser::PrintAST(ASTIndex rootIdx = AST_NULL, uint32_t depth = 0) const {
    auto PrintASTList = [&](ASTList body, uint32_t newDepth) {
        for (ASTIndex childIdx : ast.lists[body])
            PrintAST(childIdx, newDepth);
    };

    const ASTNode& root = ast.tree[rootIdx];

    switch (root.kind) {
        case ASTKind::PROGRAM: {
            PrintASTList(root.program.procedures, depth);
        } break;
        case ASTKind::PROCEDURE: {
            fmt::print(stderr, "\n");
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintASTList(root.procedure.params, depth + 1);
            PrintIndent(depth + 1);
            fmt::print(stderr, "-> {}\n\n", TypeKindName(root.procedure.retType));
            PrintASTList(root.procedure.body, depth + 1);
        } break;
        case ASTKind::IF_STATEMENT: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(root.ifstmt.cond, depth + 1);
            PrintASTList(root.ifstmt.body, depth + 1);
            if (root.ifstmt.elsestmt != AST_EMPTY) {
                PrintIndent(depth);
                fmt::print(stderr, "ELSE\n");
                PrintASTList(root.ifstmt.elsestmt, depth + 1);
            }
        } break;
        case ASTKind::FOR_STATEMENT: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            if (root.forstmt.init != AST_NULL) PrintAST(root.forstmt.init, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            if (root.forstmt.cond != AST_NULL) PrintAST(root.forstmt.cond, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            if (root.forstmt.incr != AST_NULL) PrintAST(root.forstmt.incr, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            PrintIndent(depth); fmt::print(stderr, "\n");
            PrintASTList(root.forstmt.body, depth + 1);
        } break;
        case ASTKind::RETURN_STATEMENT: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(root.ret.expr, depth + 1);
        } break;
        case ASTKind::DEFINITION: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, " ({} {}",
                (root.defn.isConst ? "let" : "mut"),
                TypeKindName(root.defn.type));
            if (root.defn.arraySize != AST_NULL) {
                fmt::print(stderr, "[]");
            }
            fmt::print(stderr, " {})\n", tokens[root.tokenIdx].text);
            if (root.defn.arraySize != AST_NULL) {
                PrintAST(root.defn.arraySize, depth + 1);
            }
            if (root.defn.initExpr != AST_NULL) {
                PrintAST(root.defn.initExpr, depth + 1);
            }
        } break;
        case ASTKind::ASSIGN: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(root.asgn.lvalue, depth + 1);
            PrintAST(root.asgn.rvalue, depth + 1);
        } break;
        case ASTKind::NEG_UNARYOP_EXPR:
        case ASTKind::NOT_UNARYOP_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(root.unaryOp.expr, depth + 1);
        } break;
        case ASTKind::EQ_BINARYOP_EXPR:
        case ASTKind::NE_BINARYOP_EXPR:
        case ASTKind::GE_BINARYOP_EXPR:
        case ASTKind::GT_BINARYOP_EXPR:
        case ASTKind::LE_BINARYOP_EXPR:
        case ASTKind::LT_BINARYOP_EXPR:
        case ASTKind::AND_BINARYOP_EXPR:
        case ASTKind::OR_BINARYOP_EXPR:
        case ASTKind::ADD_BINARYOP_EXPR:
        case ASTKind::SUB_BINARYOP_EXPR:
        case ASTKind::MUL_BINARYOP_EXPR:
        case ASTKind::DIV_BINARYOP_EXPR:
        case ASTKind::MOD_BINARYOP_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(root.binaryOp.left, depth + 1);
            PrintAST(root.binaryOp.right, depth + 1);
        } break;
        case ASTKind::INTEGER_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.i64);
        } break;
        case ASTKind::FLOAT_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.f64);
        } break;
        case ASTKind::CHAR_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.u8);
        } break;
        case ASTKind::STRING_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, " ({})\n", std::string_view{root.literal.str.buf, root.literal.str.sz});
        } break;
        case ASTKind::LVALUE_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            if (root.lvalue.subscript != AST_NULL) {
                fmt::print(stderr, "[]\n");
                PrintAST(root.lvalue.subscript, depth + 1);
            }
            else {
                fmt::print(stderr, "\n");
            }
        } break;
        case ASTKind::CALL_EXPR: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
            PrintASTList(root.call.args, depth + 1);
        } break;
        case ASTKind::CONTINUE_STATEMENT:
        case ASTKind::BREAK_STATEMENT: {
            PrintIndent(depth);
            PrintNode(rootIdx);
            fmt::print(stderr, "\n");
        } break;

        case ASTKind::UNINITIALIZED:
        case ASTKind::COUNT: assert(0); break;
    }
}


ASTIndex Parser::ParseSubscript() {
    const Token& lsquare = PeekCurrentToken();
    if (lsquare.kind != TokenKind::LSQUARE)
        return AST_NULL;
    
    // It is a subscript
    ++tokenIdx;
    ASTIndex subscript = ParseExpression();
    if (subscript == AST_NULL)
        CompileErrorAt(lsquare, "Invalid subscript, expected expression after \"[\"");
    if (PollCurrentToken().kind != TokenKind::RSQUARE)
        CompileErrorAt(lsquare, "Unmatched square bracket");
    
    return subscript;
}

std::pair<TypeKind, ASTIndex> Parser::ParseType() {
    const Token& typeTok = PeekCurrentToken();
    if (!(typeTok.kind == TokenKind::I64 || typeTok.kind == TokenKind::F64 || typeTok.kind == TokenKind::U8))
        return { TypeKind::NONE, AST_NULL };

    // It is a type
    ++tokenIdx;

    ASTIndex arrSize = ParseSubscript();
    switch (typeTok.kind) {
        case TokenKind::I64: return { TypeKind::I64, arrSize };
        case TokenKind::F64: return { TypeKind::F64, arrSize };
        case TokenKind::U8:  return { TypeKind::U8 , arrSize };
        default: assert(0 && "Unreachable"); return {};
    }
}

ASTIndex Parser::ParseVarDefn() {
    const Token& letOrMut = PeekCurrentToken();
    if (letOrMut.kind != TokenKind::LET && letOrMut.kind != TokenKind::MUT)
        return AST_NULL;

    // It is a variable definition
    ++tokenIdx;

    auto [type, arrSize] = ParseType();
    if (type == TypeKind::NONE)
        CompileErrorAt(letOrMut,
            "Invalid variable declaration, expected type after \"let\"/\"mut\"");

    ExpectAndConsumeToken(TokenKind::IDENTIFIER,
        "Invalid variable declaration, expected identifier after type");

    ASTIndex defn = NewNodeFromLastToken(ASTKind::DEFINITION);
    ast.tree[defn].defn.isConst = letOrMut.kind == TokenKind::LET;
    ast.tree[defn].defn.type = type;
    ast.tree[defn].defn.arraySize = arrSize;

    return defn;
}

ASTIndex Parser::ParseVarDefnAsgn() {
    ASTIndex defn = ParseVarDefn();
    if (defn == AST_NULL)
        return AST_NULL;

    const Token& maybeAsgn = PeekCurrentToken();

    if (maybeAsgn.kind == TokenKind::OPERATOR_ASSIGN) {
        ++tokenIdx;
        ASTIndex expr = ParseExpression();
        if (expr == AST_NULL)
            CompileErrorAt(maybeAsgn, "Expected expression after \"=\"");
        ast.tree[defn].defn.initExpr = expr;
    }

    return defn;
}

ASTIndex Parser::ParseIf() {
    const Token& token = PeekCurrentToken();
    if (token.kind != TokenKind::IF)
        return AST_NULL;

    // It is an if statement
    ++tokenIdx;
    ASTIndex ifStmt = NewNodeFromLastToken(ASTKind::IF_STATEMENT);

    const Token& lparen = PollCurrentToken();
    if (lparen.kind != TokenKind::LPAREN)
        CompileErrorAt(lparen, "Expected \"(\" after \"if\"");
    ASTIndex cond = ParseExpression();
    if (cond == AST_NULL)
        CompileErrorAt(PeekCurrentToken(), "Invalid expression after \"(\"");
    ast.tree[ifStmt].ifstmt.cond = cond;

    if (PollCurrentToken().kind != TokenKind::RPAREN)
        CompileErrorAt(lparen, "Unmatched parenthesis after \"if\" statement");
    ast.tree[ifStmt].ifstmt.body = ParseBody();

    // Parse optional else
    const Token& maybeElse = PeekCurrentToken();
    if (maybeElse.kind == TokenKind::ELSE) {
        ++tokenIdx;
        ast.tree[ifStmt].ifstmt.elsestmt = ParseBody();
    }

    return ifStmt;
}

ASTIndex Parser::ParseCall() {
    TokenIndex oldTokenIdx = tokenIdx;
    const Token& id = PeekCurrentToken();
    if (id.kind != TokenKind::IDENTIFIER)
        return AST_NULL;
    
    ++tokenIdx;
    const Token& lparen = PeekCurrentToken();
    if (lparen.kind != TokenKind::LPAREN) {
        tokenIdx = oldTokenIdx;
        return AST_NULL;
    }

    // It is a procedure call
    ++tokenIdx;
    ASTIndex call = NewNodeFromToken(oldTokenIdx, ASTKind::CALL_EXPR);

    const Token& nextTok = PeekCurrentToken();
    if (nextTok.kind == TokenKind::RPAREN) {
        // No arguments, done
        ++tokenIdx;
        return call;
    }

    ast.tree[call].call.args = NewASTList();

    // At least one argument, continue
    while (true) {
        ASTIndex arg = ParseExpression();
        if (arg == AST_NULL)
            CompileErrorAt(PeekCurrentToken(),
                "Invalid argument, excpected comma separated expression list");

        AddToASTList(ast.tree[call].call.args, arg);

        const Token& commaOrClose = PollCurrentToken();
        if (commaOrClose.kind == TokenKind::RPAREN) {
            break;
        }
        if (commaOrClose.kind != TokenKind::OPERATOR_COMMA) {
            CompileErrorAt(commaOrClose, "Unexpected token, excpected \",\" or \")\") in argument list");
        }
    }

    return call;
}


ASTIndex Parser::ParseLValue() {
    const Token& id = PeekCurrentToken();
    if (id.kind != TokenKind::IDENTIFIER)
        return AST_NULL;
    
    // It is an LValue
    ++tokenIdx;

    ASTIndex lvalue = NewNodeFromLastToken(ASTKind::LVALUE_EXPR);
    ast.tree[lvalue].lvalue.subscript = ParseSubscript();

    return lvalue;
}

ASTIndex Parser::ParseAssignment() {
    TokenIndex oldTokenIdx = tokenIdx;

    ASTIndex lvalue = ParseLValue();
    if (lvalue == AST_NULL)
        return AST_NULL;
    
    const Token& eq = PeekCurrentToken();
    if (eq.kind != TokenKind::OPERATOR_ASSIGN) {
        tokenIdx = oldTokenIdx;
        return AST_NULL;
    }
    
    // It is an assignment
    ++tokenIdx;
    
    ASTIndex rvalue = ParseExpression();
    if (rvalue == AST_NULL)
        CompileErrorAt(eq, "Expected expression after \"=\"");

    ASTIndex assign = NewNodeFromLastToken(ASTKind::ASSIGN);
    ast.tree[assign].asgn.lvalue = lvalue;
    ast.tree[assign].asgn.rvalue = rvalue;

    return assign;
}

ASTIndex Parser::ParseFor() {
    const Token& token = PeekCurrentToken();
    if (token.kind != TokenKind::FOR)
        return AST_NULL;

    // It is a for statement
    ++tokenIdx;
    ASTIndex forStmt = NewNodeFromLastToken(ASTKind::FOR_STATEMENT);

    const Token& lparen = PollCurrentToken();
    if (lparen.kind != TokenKind::LPAREN)
        CompileErrorAt(lparen, "Expected \"(\" after \"for\"");
    ASTIndex init = ParseVarDefnAsgn();
    if (init == AST_NULL) {
        const Token& semi = PeekCurrentToken();
        if (semi.kind != TokenKind::SEMICOLON)
            CompileErrorAt(semi, "Invalid declaration");
    }
    ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after for statement initializer");
    ast.tree[forStmt].forstmt.init = init;

    ASTIndex cond = ParseExpression();
    if (cond == AST_NULL) {
        const Token& semi = PeekCurrentToken();
        if (semi.kind != TokenKind::SEMICOLON)
            CompileErrorAt(semi, "Invalid condition");
    }
    ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after for statement condition");
    ast.tree[forStmt].forstmt.cond = cond;

    ASTIndex incr = ParseAssignment();
    if (incr == AST_NULL) {
        const Token& rparen = PeekCurrentToken();
        if (rparen.kind != TokenKind::RPAREN)
            CompileErrorAt(rparen, "Invalid assignment");
    }
    if (PollCurrentToken().kind != TokenKind::RPAREN)
        CompileErrorAt(lparen, "Unmatched parenthesis after \"for\" statement");
    ast.tree[forStmt].forstmt.incr = incr;

    ast.tree[forStmt].forstmt.body = ParseBody();

    return forStmt;
}

ASTIndex Parser::ParseFactor() {
    const Token& tok = PeekCurrentToken();
    if (tok.kind == TokenKind::INTEGER_LITERAL) {
        ++tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(ASTKind::INTEGER_LITERAL_EXPR);
        if (!ParseLiteral(tok.text, ast.tree[lit].literal.i64))
            CompileErrorAt(tok, "Invalid integer literal");
        return lit;
    }
    else if (tok.kind == TokenKind::FLOAT_LITERAL) {
        ++tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(ASTKind::FLOAT_LITERAL_EXPR);
        if (!ParseLiteral(tok.text, ast.tree[lit].literal.f64))
            CompileErrorAt(tok, "Invalid float literal");
        return lit;
    }
    else if (tok.kind == TokenKind::STRING_LITERAL) {
        ++tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(ASTKind::STRING_LITERAL_EXPR);
        // TODO: new string with escaped characters
        ast.tree[lit].literal.str = { .buf = tok.text.data(), .sz = tok.text.size() };
        return lit;
    }
    else if (tok.kind == TokenKind::CHAR_LITERAL) {
        ++tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(ASTKind::CHAR_LITERAL_EXPR);
        ast.tree[lit].literal.u8 = tok.text.size() == 1 ? static_cast<uint8_t>(tok.text[0]) : (
            tok.text[1] == 'n' ? '\n' :
            tok.text[1] == 't' ? '\t' :
            tok.text[1] == '"' ? '\"' :
            tok.text[1] == '\'' ? '\'' : (assert(0), 0)
        );
        return lit;
    }
    else if (tok.kind == TokenKind::OPERATOR_NEG) {
        ++tokenIdx;
        ASTIndex neg = NewNodeFromLastToken(ASTKind::NEG_UNARYOP_EXPR);
        ASTIndex expr = ParseFactor();
        if (expr == AST_NULL)
            CompileErrorAt(tok, "Expected factor after \"-\"");
        ast.tree[neg].unaryOp.expr = expr;
        return neg;
    }
    else if (tok.kind == TokenKind::OPERATOR_NOT) {
        ++tokenIdx;
        ASTIndex neg = NewNodeFromLastToken(ASTKind::NOT_UNARYOP_EXPR);
        ASTIndex expr = ParseFactor();
        if (expr == AST_NULL)
            CompileErrorAt(tok, "Expected factor after \"!\"");
        ast.tree[neg].unaryOp.expr = expr;
        return neg;
    }
    else if (tok.kind == TokenKind::LPAREN) {
        // Parenthesized expression
        ++tokenIdx;
        ASTIndex expr = ParseExpression();
        if (expr == AST_NULL)
            CompileErrorAt(tok, "Expected expression after \"(\"");
        if (PollCurrentToken().kind != TokenKind::RPAREN)
            CompileErrorAt(tok, "Unmatched parenthesis");
        return expr;
    }
    else if (tok.kind == TokenKind::IDENTIFIER) {
        // LValue or ProcedureCall
        ASTIndex call = ParseCall();
        if (call != AST_NULL)
            return call;
        ASTIndex lvalue = ParseLValue();
        return lvalue;
    }
    return AST_NULL;
}

ASTIndex Parser::ParseTerm() {
    ASTIndex left = ParseFactor();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_MUL &&
            delim.kind != TokenKind::OPERATOR_DIV &&
            delim.kind != TokenKind::OPERATOR_MOD)
        {
            break;
        }
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseFactor();
        if (right == AST_NULL) {
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromToken(op,
            delim.kind == TokenKind::OPERATOR_MUL ? ASTKind::MUL_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_DIV ? ASTKind::DIV_BINARYOP_EXPR : ASTKind::MOD_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

ASTIndex Parser::ParseValue() {
    ASTIndex left = ParseTerm();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_POS && delim.kind != TokenKind::OPERATOR_NEG)
            break;
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseTerm();
        if (right == AST_NULL) {
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromToken(op, delim.kind == TokenKind::OPERATOR_POS ?
            ASTKind::ADD_BINARYOP_EXPR : ASTKind::SUB_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

ASTIndex Parser::ParseComparison() {
    ASTIndex left = ParseValue();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_GE && delim.kind != TokenKind::OPERATOR_LE &&
            delim.kind != TokenKind::OPERATOR_GT && delim.kind != TokenKind::OPERATOR_LT)
        {
            break;
        }
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseValue();
        if (right == AST_NULL) {
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromLastToken(
            delim.kind == TokenKind::OPERATOR_GE ? ASTKind::GE_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_LE ? ASTKind::LE_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_GT ? ASTKind::GT_BINARYOP_EXPR : ASTKind::LT_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

ASTIndex Parser::ParseLogicalFactor() {
    ASTIndex left = ParseComparison();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_EQ && delim.kind != TokenKind::OPERATOR_NE)
            break;
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseComparison();
        if (right == AST_NULL) {
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromLastToken(delim.kind == TokenKind::OPERATOR_EQ ?
            ASTKind::EQ_BINARYOP_EXPR : ASTKind::NE_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

ASTIndex Parser::ParseLogicalTerm() {
    ASTIndex left = ParseLogicalFactor();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_AND)
            break;
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseLogicalFactor();
        if (right == AST_NULL) {
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromLastToken(ASTKind::AND_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

// let i64 x = 4 % 1 + !"asd" / 2.0 && a > -(8 + 2) * 3;
// TODO: Pratt parsing for binary operators, remove repetitive functions
ASTIndex Parser::ParseExpression() {
    ASTIndex left = ParseLogicalTerm();
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken();
        if (delim.kind != TokenKind::OPERATOR_OR)
            break;
        TokenIndex op = tokenIdx++;
        ASTIndex right = ParseLogicalTerm();
        if (right == AST_NULL) {
            // TODO: Should an error be thrown here?
            // In this case, token index does not have to be saved
            // TODO: Compare existing error messages to clang and gcc and improve where necessary
            tokenIdx = op;
            return AST_NULL;
        }
        ASTIndex binop = NewNodeFromLastToken(ASTKind::OR_BINARYOP_EXPR);
        ast.tree[binop].binaryOp.left = left;
        ast.tree[binop].binaryOp.right = right;
        left = binop;
    }
    return left;
}

ASTIndex Parser::ParseStatement() {
    const Token& tok = PeekCurrentToken();
    if (tok.kind == TokenKind::IF) {
        return ParseIf();
    }
    else if (tok.kind == TokenKind::FOR) {
        return ParseFor();
    }
    else if (tok.kind == TokenKind::LET || tok.kind == TokenKind::MUT) {
        ASTIndex defn = ParseVarDefnAsgn();
        // FIXME: report error at the actual token
        ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after variable definition");
        return defn;
    }
    else if (tok.kind == TokenKind::RETURN) {
        ++tokenIdx;
        ASTIndex ret = NewNodeFromLastToken(ASTKind::RETURN_STATEMENT);
        ast.tree[ret].ret.expr = ParseExpression();
        // FIXME: report error at the actual token
        ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after return");
        return ret;
    }
    else if (tok.kind == TokenKind::BREAK) {
        ++tokenIdx;
        ASTIndex brk = NewNodeFromLastToken(ASTKind::BREAK_STATEMENT);
        // FIXME: report error at the actual token
        ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after break");
        return brk;
    }
    else if (tok.kind == TokenKind::CONTINUE) {
        ++tokenIdx;
        ASTIndex cont = NewNodeFromLastToken(ASTKind::CONTINUE_STATEMENT);
        // FIXME: report error at the actual token
        ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after continue");
        return cont;
    }
    // Maybe ambiguous here
    else if (tok.kind == TokenKind::IDENTIFIER) {
        // Assignment to lvalue
        ASTIndex assign = ParseAssignment();
        if (assign != AST_NULL) {
            // FIXME: report error at the actual token
            ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after continue");
            return assign;
        }
    }

    // If all else failed, try parse ParseExpression
    ASTIndex expr = ParseExpression();
    if (expr == AST_NULL)
        CompileErrorAt(tok, "Invalid statement");
    ExpectAndConsumeToken(TokenKind::SEMICOLON, "Expected semicolon after expression");
    return expr;
}

ASTList Parser::ParseBody() {
    const Token& maybeCurly = PeekCurrentToken();
    ASTList body = NewASTList();

    if (maybeCurly.kind == TokenKind::LCURLY) {
        // Parse entire block
        ++tokenIdx;
        while (true) {
            const Token& nextTok = PeekCurrentToken();
            if (nextTok.kind == TokenKind::RCURLY) {
                ++tokenIdx;
                break;
            }

            ASTIndex stmt = ParseStatement();
            assert(stmt != AST_NULL);
            AddToASTList(body, stmt);
        }
    }
    else {
        // Parse single statement
        ASTIndex stmt = ParseStatement();
        assert(stmt != AST_NULL);
        AddToASTList(body, stmt);
    }
    return body;
}

ASTIndex Parser::ParseProcedure() {
    ExpectAndConsumeToken(TokenKind::PROC,
        "Invalid top level declaration, expected \"proc\"");
    ExpectAndConsumeToken(TokenKind::IDENTIFIER,
        "Expected identifier after \"proc\"");
    ASTIndex proc = NewNodeFromLastToken(ASTKind::PROCEDURE);
    ExpectAndConsumeToken(TokenKind::LPAREN,
        "Expected \"(\" after procedure name");

    const Token& nextTok = PeekCurrentToken();
    if (nextTok.kind == TokenKind::RPAREN) {
        // No parameters, do nothing
        ++tokenIdx;
    }
    else if (nextTok.kind == TokenKind::LET || nextTok.kind == TokenKind::MUT) {
        // At least one parameter, continue
        ASTList params = ast.tree[proc].procedure.params = NewASTList();
        while (true) {
            ASTIndex param = ParseVarDefn();
            if (param == AST_NULL)
                CompileErrorAt(PeekCurrentToken(),
                    "Unexpected token, excpected comma separated parameter list");

            AddToASTList(params, param);

            const Token& commaOrClose = PollCurrentToken();
            if (commaOrClose.kind == TokenKind::RPAREN) {
                break;
            }
            if (commaOrClose.kind != TokenKind::OPERATOR_COMMA) {
                CompileErrorAt(commaOrClose, "Unexpected token, excpected \",\" or \")\") in parameter list");
            }

        }
    }
    else {
        CompileErrorAt(PeekCurrentToken(), "Unexpected token after procedure definition, expected variable definition or closing parenthesis");
    }

    // Optional return type
    const Token* arrowOrCurly = &PeekCurrentToken();
    if (arrowOrCurly->kind == TokenKind::ARROW) {
        ++tokenIdx;
        auto [retType, arrSize] = ParseType();
        if (arrSize != AST_NULL) {
            CompileErrorAt(*arrowOrCurly, "Returning arrays is not allowed!");
        }

        ast.tree[proc].procedure.retType = retType;
        arrowOrCurly = &PeekCurrentToken();
    }

    ast.tree[proc].procedure.body = ParseBody();

    return proc;
}

void Parser::ParseProgram() {
    ASTIndex allProcs = NewASTList();
    while (tokenIdx < tokens.size()) {
        ASTIndex proc = ParseProcedure();
        assert(proc != AST_NULL);
        AddToASTList(allProcs, proc);
    }
    ast.tree[0].program.procedures = allProcs;
}

void Parser::ParseEntireProgram() {
    // Reserve "null indices". If a token index, ast index, or list index, then it's "null"
    // Null indices are valid if the field is optional (for instance, for statement condition)
    tokenIdx = 1;              // TokenIndex TOKEN_NULL
    NewNode(ASTKind::PROGRAM); // ASTIndex AST_NULL
    NewASTList();              // ASTList AST_EMPTY
    assert(!tokens.empty() && tokens[0].kind == TokenKind::NONE);
    assert(ast.tree.size() == 1);
    assert(ast.lists.size() == 1);

    ParseProgram();
}

AST ParseEntireProgram(File file, const std::vector<Token>& tokens) {
    AST ast;
    Parser parser{file, tokens, ast};
    parser.ParseEntireProgram();
    parser.PrintAST();
    return ast;
}
