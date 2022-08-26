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

static AST& GetAST(const Parser& parser, ASTIndex idx) {
    assert(idx != AST_NULL);
    return (*parser.mem)[idx];
}

static ASTIndex NewNode(Parser& parser, ASTKind kind) {
    parser.mem->emplace_back();
    parser.mem->back().kind = kind;
    return static_cast<ASTIndex>(parser.mem->size()-1);
}

static ASTIndex NewNodeFromToken(Parser& parser, TokenIndex tokenIdx, ASTKind kind) {
    ASTIndex idx = NewNode(parser, kind);
    AST& node = GetAST(parser, idx);
    node.tokenIdx = tokenIdx;
    return idx;
}

static ASTIndex NewNodeFromLastToken(Parser& parser, ASTKind kind) {
    return NewNodeFromToken(parser, parser.tokenIdx - 1, kind);
}

static const Token& PeekCurrentToken(const Parser& parser) {
    assert(parser.tokenIdx < parser.tokens.size());
    return parser.tokens[parser.tokenIdx];
}

static const Token& PollCurrentToken(Parser& parser) {
    assert(parser.tokenIdx < parser.tokens.size());
    return parser.tokens[parser.tokenIdx++];
}

#define CompileErrorAt(parser, token, format, ...) do { \
        fmt::print(stderr, "{}\n", CompileErrorMessage((parser).filename, (token).pos.line, (token).pos.col, \
            (parser).source, (token).pos.idx, format, __VA_OPT__(,) __VA_ARGS__)); \
        exit(1); \
    } while (0)

#define ExpectAndConsumeToken(parser, expectedKind, format, ...) do { \
        const Token& token = PollCurrentToken(parser); \
        if (token.kind != expectedKind) \
            CompileErrorAt(parser, token, format __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

static ASTList NewASTList(Parser& parser) {
    parser.indices.emplace_back();
    return static_cast<ASTList>(parser.indices.size() - 1);
}

static void AddToASTList(Parser& parser, ASTList list, ASTIndex idx) {
    parser.indices[list].push_back(idx);
}

static void PrintIndent(uint32_t depth) {
    for (uint32_t i = 0; i < depth; ++i)
        fmt::print(stderr, "    ");
}

static void PrintToken(const Token& token) {
    fmt::print(stderr, "{}:{}:{}:\"{}\"", TokenKindName(token.kind), token.pos.line, token.pos.col, token.text);
}

static void PrintToken(const Parser& parser, TokenIndex tokenIdx) {
    const Token& token = parser.tokens[tokenIdx];
    PrintToken(token);
}

static void PrintNode(const Parser& parser, ASTIndex rootIdx) {
    const AST& root = GetAST(parser, rootIdx);
    fmt::print(stderr, "{}: ", ASTKindName(root.kind));
    assert(root.tokenIdx != TOKEN_NULL);
    const Token& token = parser.tokens[root.tokenIdx];
    PrintToken(token);
}

static void PrintAST(const Parser& parser, ASTIndex rootIdx = AST_NULL, uint32_t depth = 0) {
    auto PrintASTList = [&parser](ASTList body, uint32_t newDepth) {
        for (ASTIndex childIdx : parser.indices[body])
            PrintAST(parser, childIdx, newDepth);
    };

    const AST& root = (*parser.mem)[rootIdx];

    switch (root.kind) {
        case ASTKind::PROGRAM: {
            PrintASTList(root.program.procedures, depth);
        } break;
        case ASTKind::PROCEDURE: {
            fmt::print(stderr, "\n");
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintASTList(root.proc.params, depth + 1);
            PrintIndent(depth + 1);
            fmt::print(stderr, "-> {}\n\n", TypeKindName(root.proc.retType));
            PrintASTList(root.proc.body, depth + 1);
        } break;
        case ASTKind::IF_STATEMENT: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(parser, root.ifstmt.cond, depth + 1);
            PrintASTList(root.ifstmt.body, depth + 1);
            if (root.ifstmt.elsestmt != AST_EMPTY) {
                PrintIndent(depth);
                fmt::print(stderr, "ELSE\n");
                PrintASTList(root.ifstmt.elsestmt, depth + 1);
            }
        } break;
        case ASTKind::FOR_STATEMENT: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            if (root.forstmt.init) PrintAST(parser, root.forstmt.init, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            if (root.forstmt.cond) PrintAST(parser, root.forstmt.cond, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            if (root.forstmt.incr) PrintAST(parser, root.forstmt.incr, depth + 1);
            else { PrintIndent(depth + 1); fmt::print(stderr, "-\n"); }
            PrintIndent(depth); fmt::print(stderr, "\n");
            PrintASTList(root.forstmt.body, depth + 1);
        } break;
        case ASTKind::RETURN_STATEMENT: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(parser, root.ret.expr, depth + 1);
        } break;
        case ASTKind::DEFINITION: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, " ({} {}",
                (root.defn.isConst ? "let" : "mut"),
                TypeKindName(root.defn.type));
            if (root.defn.arraySize != AST_NULL) {
                fmt::print(stderr, "[]");
            }
            fmt::print(stderr, " {})\n", parser.tokens[root.tokenIdx].text);
            if (root.defn.arraySize != AST_NULL) {
                PrintAST(parser, root.defn.arraySize, depth + 1);
            }
            if (root.defn.initExpr != AST_NULL) {
                PrintAST(parser, root.defn.initExpr, depth + 1);
            }
        } break;
        case ASTKind::ASSIGN: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(parser, root.asgn.lvalue, depth + 1);
            PrintAST(parser, root.asgn.rvalue, depth + 1);
        } break;
        case ASTKind::NEG_UNARYOP_EXPR:
        case ASTKind::NOT_UNARYOP_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(parser, root.unaryOp.expr, depth + 1);
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
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintAST(parser, root.binaryOp.left, depth + 1);
            PrintAST(parser, root.binaryOp.right, depth + 1);
        } break;
        case ASTKind::INTEGER_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.i64);
        } break;
        case ASTKind::FLOAT_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.f64);
        } break;
        case ASTKind::CHAR_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, " ({})\n", root.literal.u8);
        } break;
        case ASTKind::STRING_LITERAL_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, " ({})\n", std::string_view{root.literal.str.buf, root.literal.str.sz});
        } break;
        case ASTKind::LVALUE_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            if (root.lvalue.subscript != AST_NULL) {
                fmt::print(stderr, "[]\n");
                PrintAST(parser, root.lvalue.subscript, depth + 1);
            }
            else {
                fmt::print(stderr, "\n");
            }
        } break;
        case ASTKind::CALL_EXPR: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
            PrintASTList(root.call.args, depth + 1);
        } break;
        case ASTKind::CONTINUE_STATEMENT:
        case ASTKind::BREAK_STATEMENT: {
            PrintIndent(depth);
            PrintNode(parser, rootIdx);
            fmt::print(stderr, "\n");
        } break;

        case ASTKind::UNINITIALIZED:
        case ASTKind::COUNT: assert(0); break;
    }
}


static ASTIndex ParseExpression(Parser& parser);
static ASTIndex ParseBody(Parser& parser);

static ASTIndex ParseSubscript(Parser& parser) {
    const Token& lsquare = PeekCurrentToken(parser);
    if (lsquare.kind != TokenKind::LSQUARE)
        return AST_NULL;
    
    // It is a subscript
    ++parser.tokenIdx;
    ASTIndex subscript = ParseExpression(parser);
    if (subscript == AST_NULL)
        CompileErrorAt(parser, lsquare, "Invalid subscript, expected expression after \"[\"");
    if (PollCurrentToken(parser).kind != TokenKind::RSQUARE)
        CompileErrorAt(parser, lsquare, "Unmatched square bracket");
    
    return subscript;
}

static std::pair<TypeKind, ASTIndex> ParseType(Parser& parser) {
    static auto isPrimitiveType = [](const Token& token) {
        return token.kind == TokenKind::I64 ||
                token.kind == TokenKind::F64 ||
                token.kind == TokenKind::U8;
    };

    const Token& typeTok = PeekCurrentToken(parser);
    if (!isPrimitiveType(typeTok))
        return { TypeKind::NONE, 0 };

    // It is a type
    ++parser.tokenIdx;

    ASTIndex arrSize = ParseSubscript(parser);

    switch (typeTok.kind) {
        case TokenKind::I64: return { TypeKind::I64, arrSize };
        case TokenKind::F64: return { TypeKind::F64, arrSize };
        case TokenKind::U8:  return { TypeKind::U8 , arrSize };
        default: assert(0 && "Unreachable"); return {};
    }
}

static ASTIndex ParseVarDefn(Parser& parser) {
    const Token& letOrMut = PeekCurrentToken(parser);
    if (letOrMut.kind != TokenKind::LET && letOrMut.kind != TokenKind::MUT)
        return AST_NULL;

    // It is a variable definition
    ++parser.tokenIdx;

    auto [type, arrSize] = ParseType(parser);
    if (type == TypeKind::NONE)
        CompileErrorAt(parser, letOrMut,
            "Invalid variable declaration, expected type after \"let\"/\"mut\"");

    ExpectAndConsumeToken(parser, TokenKind::IDENTIFIER,
        "Invalid variable declaration, expected identifier after type");

    ASTIndex defn = NewNodeFromLastToken(parser, ASTKind::DEFINITION);
    GetAST(parser, defn).defn.isConst = letOrMut.kind == TokenKind::LET;
    GetAST(parser, defn).defn.type = type;
    GetAST(parser, defn).defn.arraySize = arrSize;

    return defn;
}

static ASTIndex ParseVarDefnAsgn(Parser& parser) {
    ASTIndex defn = ParseVarDefn(parser);
    if (defn == AST_NULL)
        return AST_NULL;

    const Token& maybeAsgn = PeekCurrentToken(parser);

    if (maybeAsgn.kind == TokenKind::OPERATOR_ASSIGN) {
        ++parser.tokenIdx;
        ASTIndex expr = ParseExpression(parser);
        if (expr == AST_NULL)
            CompileErrorAt(parser, maybeAsgn, "Expected expression after \"=\"");
        GetAST(parser, defn).defn.initExpr = expr;
    }

    return defn;
}

static ASTIndex ParseIf(Parser& parser) {
    const Token& token = PeekCurrentToken(parser);
    if (token.kind != TokenKind::IF)
        return AST_NULL;

    // It is an if statement
    ++parser.tokenIdx;
    ASTIndex ifStmt = NewNodeFromLastToken(parser, ASTKind::IF_STATEMENT);

    const Token& lparen = PollCurrentToken(parser);
    if (lparen.kind != TokenKind::LPAREN)
        CompileErrorAt(parser, lparen, "Expected \"(\" after \"if\"");
    ASTIndex cond = ParseExpression(parser);
    if (cond == AST_NULL)
        CompileErrorAt(parser, PeekCurrentToken(parser), "Invalid expression after \"(\"");
    GetAST(parser, ifStmt).ifstmt.cond = cond;

    if (PollCurrentToken(parser).kind != TokenKind::RPAREN)
        CompileErrorAt(parser, lparen, "Unmatched parenthesis after \"if\" statement");
    GetAST(parser, ifStmt).ifstmt.body = ParseBody(parser);

    // Parse optional else
    const Token& maybeElse = PeekCurrentToken(parser);
    if (maybeElse.kind == TokenKind::ELSE) {
        ++parser.tokenIdx;
        GetAST(parser, ifStmt).ifstmt.elsestmt = ParseBody(parser);
    }

    return ifStmt;
}

static ASTIndex ParseCall(Parser& parser) {
    TokenIndex oldTokenIdx = parser.tokenIdx;
    const Token& id = PeekCurrentToken(parser);
    if (id.kind != TokenKind::IDENTIFIER)
        return AST_NULL;
    
    ++parser.tokenIdx;
    const Token& lparen = PeekCurrentToken(parser);
    if (lparen.kind != TokenKind::LPAREN) {
        parser.tokenIdx = oldTokenIdx;
        return AST_NULL;
    }

    // It is a procedure call
    ++parser.tokenIdx;
    ASTIndex call = NewNodeFromToken(parser, oldTokenIdx, ASTKind::CALL_EXPR);

    const Token& nextTok = PeekCurrentToken(parser);
    if (nextTok.kind == TokenKind::RPAREN) {
        // No arguments, done
        ++parser.tokenIdx;
        return call;
    }

    GetAST(parser, call).call.args = NewASTList(parser);

    // At least one argument, continue
    while (true) {
        ASTIndex arg = ParseExpression(parser);
        if (arg == AST_NULL)
            CompileErrorAt(parser, PeekCurrentToken(parser),
                "Invalid argument, excpected comma separated expression list");

        AddToASTList(parser, GetAST(parser, call).call.args, arg);

        const Token& commaOrClose = PollCurrentToken(parser);
        if (commaOrClose.kind == TokenKind::RPAREN) {
            break;
        }
        if (commaOrClose.kind != TokenKind::OPERATOR_COMMA) {
            CompileErrorAt(parser, commaOrClose, "Unexpected token, excpected \",\" or \")\") in argument list");
        }
    }

    return call;
}


static ASTIndex ParseLValue(Parser& parser) {
    const Token& id = PeekCurrentToken(parser);
    if (id.kind != TokenKind::IDENTIFIER)
        return AST_NULL;
    
    // It is an LValue
    ++parser.tokenIdx;

    ASTIndex lvalue = NewNodeFromLastToken(parser, ASTKind::LVALUE_EXPR);
    GetAST(parser, lvalue).lvalue.subscript = ParseSubscript(parser);

    return lvalue;
}

static ASTIndex ParseAssignment(Parser& parser) {
    TokenIndex oldTokenIdx = parser.tokenIdx;

    ASTIndex lvalue = ParseLValue(parser);
    if (lvalue == AST_NULL)
        return AST_NULL;
    
    const Token& eq = PeekCurrentToken(parser);
    if (eq.kind != TokenKind::OPERATOR_ASSIGN) {
        parser.tokenIdx = oldTokenIdx;
        return AST_NULL;
    }
    
    // It is an assignment
    ++parser.tokenIdx;
    
    ASTIndex rvalue = ParseExpression(parser);
    if (rvalue == AST_NULL)
        CompileErrorAt(parser, eq, "Expected expression after \"=\"");

    ASTIndex assign = NewNodeFromLastToken(parser, ASTKind::ASSIGN);
    GetAST(parser, assign).asgn.lvalue = lvalue;
    GetAST(parser, assign).asgn.rvalue = rvalue;

    return assign;
}

static ASTIndex ParseFor(Parser& parser) {
    const Token& token = PeekCurrentToken(parser);
    if (token.kind != TokenKind::FOR)
        return AST_NULL;

    // It is a for statement
    ++parser.tokenIdx;
    ASTIndex forStmt = NewNodeFromLastToken(parser, ASTKind::FOR_STATEMENT);

    const Token& lparen = PollCurrentToken(parser);
    if (lparen.kind != TokenKind::LPAREN)
        CompileErrorAt(parser, lparen, "Expected \"(\" after \"for\"");
    ASTIndex init = ParseVarDefnAsgn(parser);
    if (init == AST_NULL) {
        const Token& semi = PeekCurrentToken(parser);
        if (semi.kind != TokenKind::SEMICOLON)
            CompileErrorAt(parser, semi, "Invalid declaration");
    }
    ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after for statement initializer");
    GetAST(parser, forStmt).forstmt.init = init;

    ASTIndex cond = ParseExpression(parser);
    if (cond == AST_NULL) {
        const Token& semi = PeekCurrentToken(parser);
        if (semi.kind != TokenKind::SEMICOLON)
            CompileErrorAt(parser, semi, "Invalid condition");
    }
    ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after for statement condition");
    GetAST(parser, forStmt).forstmt.cond = cond;

    ASTIndex incr = ParseAssignment(parser);
    if (incr == AST_NULL) {
        const Token& rparen = PeekCurrentToken(parser);
        if (rparen.kind != TokenKind::RPAREN)
            CompileErrorAt(parser, rparen, "Invalid assignment");
    }
    if (PollCurrentToken(parser).kind != TokenKind::RPAREN)
        CompileErrorAt(parser, lparen, "Unmatched parenthesis after \"for\" statement");
    GetAST(parser, forStmt).forstmt.incr = incr;

    GetAST(parser, forStmt).forstmt.body = ParseBody(parser);

    return forStmt;
}

static ASTIndex ParseFactor(Parser& parser) {
    const Token& tok = PeekCurrentToken(parser);
    if (tok.kind == TokenKind::INTEGER_LITERAL) {
        ++parser.tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(parser, ASTKind::INTEGER_LITERAL_EXPR);
        if (!ParseLiteral(tok.text, GetAST(parser, lit).literal.i64))
            CompileErrorAt(parser, tok, "Invalid integer literal");
        return lit;
    }
    else if (tok.kind == TokenKind::FLOAT_LITERAL) {
        ++parser.tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(parser, ASTKind::FLOAT_LITERAL_EXPR);
        if (!ParseLiteral(tok.text, GetAST(parser, lit).literal.f64))
            CompileErrorAt(parser, tok, "Invalid float literal");
        return lit;
    }
    else if (tok.kind == TokenKind::STRING_LITERAL) {
        ++parser.tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(parser, ASTKind::STRING_LITERAL_EXPR);
        // TODO: new string with escaped characters
        GetAST(parser, lit).literal.str = { .buf = tok.text.data(), .sz = tok.text.size() };
        return lit;
    }
    else if (tok.kind == TokenKind::CHAR_LITERAL) {
        ++parser.tokenIdx;
        ASTIndex lit = NewNodeFromLastToken(parser, ASTKind::CHAR_LITERAL_EXPR);
        GetAST(parser, lit).literal.u8 = tok.text.size() == 1 ? tok.text[0] : (
            tok.text[1] == 'n' ? '\n' :
            tok.text[1] == 't' ? '\t' :
            tok.text[1] == '"' ? '\"' :
            tok.text[1] == '\'' ? '\'' : (assert(0), 0)
        );
        return lit;
    }
    else if (tok.kind == TokenKind::OPERATOR_NEG) {
        ++parser.tokenIdx;
        ASTIndex neg = NewNodeFromLastToken(parser, ASTKind::NEG_UNARYOP_EXPR);
        ASTIndex expr = ParseFactor(parser);
        if (expr == AST_NULL)
            CompileErrorAt(parser, tok, "Expected factor after \"-\"");
        GetAST(parser, neg).unaryOp.expr = expr;
        return neg;
    }
    else if (tok.kind == TokenKind::OPERATOR_NOT) {
        ++parser.tokenIdx;
        ASTIndex neg = NewNodeFromLastToken(parser, ASTKind::NOT_UNARYOP_EXPR);
        ASTIndex expr = ParseFactor(parser);
        if (expr == AST_NULL)
            CompileErrorAt(parser, tok, "Expected factor after \"!\"");
        GetAST(parser, neg).unaryOp.expr = expr;
        return neg;
    }
    else if (tok.kind == TokenKind::LPAREN) {
        // Parenthesized expression
        ++parser.tokenIdx;
        ASTIndex expr = ParseExpression(parser);
        if (expr == AST_NULL)
            CompileErrorAt(parser, tok, "Expected expression after \"(\"");
        if (PollCurrentToken(parser).kind != TokenKind::RPAREN)
            CompileErrorAt(parser, tok, "Unmatched parenthesis");
        return expr;
    }
    else if (tok.kind == TokenKind::IDENTIFIER) {
        // LValue or ProcedureCall
        ASTIndex call = ParseCall(parser);
        if (call != AST_NULL)
            return call;
        ASTIndex lvalue = ParseLValue(parser);
        return lvalue;
    }
    return AST_NULL;
}

static ASTIndex ParseTerm(Parser& parser) {
    ASTIndex left = ParseFactor(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_MUL &&
            delim.kind != TokenKind::OPERATOR_DIV &&
            delim.kind != TokenKind::OPERATOR_MOD)
        {
            break;
        }
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser,
            delim.kind == TokenKind::OPERATOR_MUL ? ASTKind::MUL_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_DIV ? ASTKind::DIV_BINARYOP_EXPR : ASTKind::MOD_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseFactor(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}

static ASTIndex ParseValue(Parser& parser) {
    ASTIndex left = ParseTerm(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_POS && delim.kind != TokenKind::OPERATOR_NEG)
            break;
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser, delim.kind == TokenKind::OPERATOR_POS ?
            ASTKind::ADD_BINARYOP_EXPR : ASTKind::SUB_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseTerm(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}


static ASTIndex ParseComparison(Parser& parser) {
    ASTIndex left = ParseValue(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_GE && delim.kind != TokenKind::OPERATOR_LE &&
            delim.kind != TokenKind::OPERATOR_GT && delim.kind != TokenKind::OPERATOR_LT)
        {
            break;
        }
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser,
            delim.kind == TokenKind::OPERATOR_GE ? ASTKind::GE_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_LE ? ASTKind::LE_BINARYOP_EXPR :
            delim.kind == TokenKind::OPERATOR_GT ? ASTKind::GT_BINARYOP_EXPR : ASTKind::LT_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseValue(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}

static ASTIndex ParseLogicalFactor(Parser& parser) {
    ASTIndex left = ParseComparison(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_EQ && delim.kind != TokenKind::OPERATOR_NE)
            break;
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser, delim.kind == TokenKind::OPERATOR_EQ ?
            ASTKind::EQ_BINARYOP_EXPR : ASTKind::NE_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseComparison(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}

static ASTIndex ParseLogicalTerm(Parser& parser) {
    ASTIndex left = ParseLogicalFactor(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_AND)
            break;
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser, ASTKind::AND_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseLogicalFactor(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}

// let i64 x = 4 % 1 + !"asd" / 2.0 && a > -(8 + 2) * 3;
static ASTIndex ParseExpression(Parser& parser) {
    ASTIndex left = ParseLogicalTerm(parser);
    if (left == AST_NULL)
        return AST_NULL;
    while (true) {
        const Token& delim = PeekCurrentToken(parser);
        if (delim.kind != TokenKind::OPERATOR_OR)
            break;
        ++parser.tokenIdx;
        ASTIndex binop = NewNodeFromLastToken(parser, ASTKind::OR_BINARYOP_EXPR);
        GetAST(parser, binop).binaryOp.left = left;
        ASTIndex right = ParseLogicalTerm(parser);
        if (right == AST_NULL)
            return AST_NULL;
        GetAST(parser, binop).binaryOp.right = right;
        left = binop;
    }
    return left;
}

static ASTIndex ParseStatement(Parser& parser) {
    const Token& tok = PeekCurrentToken(parser);
    if (tok.kind == TokenKind::IF) {
        return ParseIf(parser);
    }
    else if (tok.kind == TokenKind::FOR) {
        return ParseFor(parser);
    }
    else if (tok.kind == TokenKind::LET || tok.kind == TokenKind::MUT) {
        ASTIndex defn = ParseVarDefnAsgn(parser);
        ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after variable definition");
        return defn;
    }
    else if (tok.kind == TokenKind::RETURN) {
        ++parser.tokenIdx;
        ASTIndex ret = NewNodeFromLastToken(parser, ASTKind::RETURN_STATEMENT);
        GetAST(parser, ret).ret.expr = ParseExpression(parser);
        ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after return");
        return ret;
    }
    else if (tok.kind == TokenKind::BREAK) {
        ++parser.tokenIdx;
        ASTIndex brk = NewNodeFromLastToken(parser, ASTKind::BREAK_STATEMENT);
        ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after break");
        return brk;
    }
    else if (tok.kind == TokenKind::CONTINUE) {
        ++parser.tokenIdx;
        ASTIndex cont = NewNodeFromLastToken(parser, ASTKind::CONTINUE_STATEMENT);
        ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after continue");
        return cont;
    }
    // Maybe ambiguous here
    else if (tok.kind == TokenKind::IDENTIFIER) {
        // Assignment to lvalue
        ASTIndex assign = ParseAssignment(parser);
        if (assign != AST_NULL) {
            ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after continue");
            return assign;
        }
    }

    // If all else failed, try parse ParseExpression
    ASTIndex expr = ParseExpression(parser);
    if (expr == AST_NULL)
        CompileErrorAt(parser, tok, "Invalid statement");
    ExpectAndConsumeToken(parser, TokenKind::SEMICOLON, "Expected semicolon after expression");
    return expr;
}

static ASTList ParseBody(Parser& parser) {
    const Token& maybeCurly = PeekCurrentToken(parser);
    ASTList body = NewASTList(parser);

    if (maybeCurly.kind == TokenKind::LCURLY) {
        // Parse entire block
        ++parser.tokenIdx;
        while (true) {
            const Token& nextTok = PeekCurrentToken(parser);
            if (nextTok.kind == TokenKind::RCURLY) {
                ++parser.tokenIdx;
                break;
            }

            ASTIndex stmt = ParseStatement(parser);
            assert(stmt != AST_NULL);
            AddToASTList(parser, body, stmt);
        }
    }
    else {
        // Parse single statement
        ASTIndex stmt = ParseStatement(parser);
        assert(stmt != AST_NULL);
        AddToASTList(parser, body, stmt);
    }
    return body;
}

static ASTIndex ParseProcedure(Parser& parser) {
    ExpectAndConsumeToken(parser, TokenKind::PROC,
        "Invalid top level declaration, expected \"proc\"");
    ExpectAndConsumeToken(parser, TokenKind::IDENTIFIER,
        "Expected identifier after \"proc\"");
    ASTIndex proc = NewNodeFromLastToken(parser, ASTKind::PROCEDURE);
    ExpectAndConsumeToken(parser, TokenKind::LPAREN,
        "Expected \"(\" after procedure name");

    const Token& nextTok = PeekCurrentToken(parser);
    if (nextTok.kind == TokenKind::RPAREN) {
        // No parameters, do nothing
        ++parser.tokenIdx;
    }
    else if (nextTok.kind == TokenKind::LET || nextTok.kind == TokenKind::MUT) {
        // At least one parameter, continue
        GetAST(parser, proc).proc.params = NewASTList(parser);
        while (true) {
            ASTIndex param = ParseVarDefn(parser);
            if (param == AST_NULL)
                CompileErrorAt(parser, PeekCurrentToken(parser),
                    "Unexpected token, excpected comma separated parameter list");

            AddToASTList(parser, GetAST(parser, proc).proc.params, param);

            const Token& commaOrClose = PollCurrentToken(parser);
            if (commaOrClose.kind == TokenKind::RPAREN) {
                break;
            }
            if (commaOrClose.kind != TokenKind::OPERATOR_COMMA) {
                CompileErrorAt(parser, commaOrClose, "Unexpected token, excpected \",\" or \")\") in parameter list");
            }

        }
    }
    else {
        CompileErrorAt(parser, PeekCurrentToken(parser), "Unexpected token after procedure definition, expected variable definition or closing parenthesis");
    }

    // Optional return type
    const Token* arrowOrCurly = &PeekCurrentToken(parser);
    if (arrowOrCurly->kind == TokenKind::ARROW) {
        ++parser.tokenIdx;
        auto [retType, arrSize] = ParseType(parser);
        if (arrSize != 0) {
            CompileErrorAt(parser, *arrowOrCurly, "Returning arrays is not allowed!");
        }

        GetAST(parser, proc).proc.retType = retType;
        arrowOrCurly = &PeekCurrentToken(parser);
    }

    GetAST(parser, proc).proc.body = ParseBody(parser);

    return proc;
}

static void ParseProgram(Parser& parser) {
    ASTIndex allProcs = NewASTList(parser);
    while (parser.tokenIdx < parser.tokens.size()) {
        ASTIndex proc = ParseProcedure(parser);
        assert(proc != AST_NULL);
        AddToASTList(parser, allProcs, proc);
    }
    (*parser.mem)[0].program.procedures = allProcs;
}

std::vector<AST> ParseEntireProgram(Parser& parser) {
    std::vector<AST> ast;
    parser.mem = &ast;
    parser.tokenIdx = 1;

    // Reserve "null indices", the 0th element is unused
    parser.indices.emplace_back();
    NewNode(parser, ASTKind::PROGRAM);
    assert(parser.tokens[0].kind == TokenKind::NONE);
    assert(parser.indices.size() == 1);
    assert(parser.mem->size() == 1);

    ParseProgram(parser);
    PrintAST(parser);

    parser.mem = nullptr;
    return ast;
}

