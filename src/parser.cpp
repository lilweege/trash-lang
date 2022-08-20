#include "parser.hpp"
#include "tokenizer.hpp"
#include "compileerror.hpp"

#include <array>
#include <cassert>

const char* ASTKindName(ASTKind kind) {
    static_assert(static_cast<uint32_t>(ASTKind::COUNT) == 31, "Exhaustive check of AST kinds failed");
    const std::array<const char*, static_cast<uint32_t>(ASTKind::COUNT)> ASTKindNames{
        "ASTKind::UNINITIALIZED",
        "ASTKind::PROGRAM",
        "ASTKind::PROCEDURE",
        "ASTKind::IF_STATEMENT",
        "ASTKind::FOR_STATEMENT",
        "ASTKind::RETURN_STATEMENT",
        "ASTKind::CONTINUE_STATEMENT",
        "ASTKind::BREAK_STATEMENT",
        "ASTKind::DEFINITION",
        "ASTKind::ASSIGN",
        "ASTKind::NEG_UNARYOP_EXPR",
        "ASTKind::NOT_UNARYOP_EXPR",
        "ASTKind::EQ_BINARYOP_EXPR",
        "ASTKind::NE_BINARYOP_EXPR",
        "ASTKind::GE_BINARYOP_EXPR",
        "ASTKind::GT_BINARYOP_EXPR",
        "ASTKind::LE_BINARYOP_EXPR",
        "ASTKind::LT_BINARYOP_EXPR",
        "ASTKind::AND_BINARYOP_EXPR",
        "ASTKind::OR_BINARYOP_EXPR",
        "ASTKind::ADD_BINARYOP_EXPR",
        "ASTKind::SUB_BINARYOP_EXPR",
        "ASTKind::MUL_BINARYOP_EXPR",
        "ASTKind::DIV_BINARYOP_EXPR",
        "ASTKind::MOD_BINARYOP_EXPR",
        "ASTKind::INTEGER_LITERAL_EXPR",
        "ASTKind::FLOAT_LITERAL_EXPR",
        "ASTKind::CHAR_LITERAL_EXPR",
        "ASTKind::STRING_LITERAL_EXPR",
        "ASTKind::LVALUE_EXPR",
        "ASTKind::CALL_EXPR",
    };
    return ASTKindNames[static_cast<uint32_t>(kind)];
}

static void PrintToken(const Token& token) {
    fmt::print(stderr, "{}:{}:{}:\"{}\"", TokenKindName(token.kind), token.pos.line, token.pos.col, token.text);
}
static void PrintToken(Parser& parser, TokenIndex tokenIdx) {
    const Token& token = parser.tokens[tokenIdx];
    PrintToken(token);
}

static void PrintIndent(uint32_t depth) {
    for (uint32_t i = 0; i < depth; ++i)
        fmt::print(stderr, "\t");
}

// FIXME: print stuff fully
static void PrintAST(const Parser& parser, ASTIndex rootIdx = 0, uint32_t depth = 0) {
    auto PrintASTList = [&parser](ASTList body, uint32_t newDepth) {
//         for (ASTIndex i = body.first; i < body.first + body.numStmts; ++i)
//             PrintAST(parser, AST::indices[i], newDepth);
    };

    const AST& root = (*parser.mem)[rootIdx];

    PrintIndent(depth);
    fmt::print(stderr, "{}: ", ASTKindName(root.kind));
    if (root.tokenIdx != 0) {
        const Token& token = parser.tokens[root.tokenIdx];
        PrintToken(token);
    }
    fmt::print(stderr, "\n");

    switch (root.kind) {
        case ASTKind::PROGRAM:
            PrintASTList(root.program.procedures, depth + 1);
            break;
        case ASTKind::PROCEDURE:
            PrintASTList(root.proc.params, depth + 1);
            PrintASTList(root.proc.body, depth + 1);
            break;
        case ASTKind::IF_STATEMENT:
            PrintAST(parser, root.ifstmt.cond, depth + 1);
            PrintASTList(root.ifstmt.body, depth + 1);
            PrintIndent(depth); fmt::print(stderr, "ELSE\n");
            PrintASTList(root.ifstmt.elsestmt, depth + 1);
            break;
        case ASTKind::FOR_STATEMENT:
            PrintAST(parser, root.forstmt.init, depth + 1);
            PrintAST(parser, root.forstmt.cond, depth + 1);
            PrintAST(parser, root.forstmt.incr, depth + 1);
            PrintASTList(root.forstmt.body, depth + 1);
            break;
        case ASTKind::RETURN_STATEMENT:
            PrintAST(parser, root.ret.expr, depth + 1);
            break;
        case ASTKind::DEFINITION:
            PrintAST(parser, root.defn.initExpr, depth + 1);
            break;
        case ASTKind::ASSIGN:
            PrintAST(parser, root.asgn.expession, depth + 1);
            PrintAST(parser, root.asgn.subscript, depth + 1);
            break;
        case ASTKind::NEG_UNARYOP_EXPR:
        case ASTKind::NOT_UNARYOP_EXPR:
            PrintAST(parser, root.unaryOp.expr, depth + 1);
            break;
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
        case ASTKind::MOD_BINARYOP_EXPR:
            PrintAST(parser, root.binaryOp.left, depth + 1);
            PrintAST(parser, root.binaryOp.right, depth + 1);
            break;
        case ASTKind::INTEGER_LITERAL_EXPR:
            PrintIndent(depth + 1); fmt::print(stderr, "{}\n", root.literal.i64);
            break;
        case ASTKind::FLOAT_LITERAL_EXPR:
            PrintIndent(depth + 1); fmt::print(stderr, "{}\n", root.literal.f64);
            break;
        case ASTKind::CHAR_LITERAL_EXPR:
            PrintIndent(depth + 1); fmt::print(stderr, "{}\n", root.literal.u8);
            break;
        case ASTKind::STRING_LITERAL_EXPR:
            PrintIndent(depth + 1); fmt::print(stderr, "{}\n", std::string_view{root.literal.str.buf, root.literal.str.sz});
            break;

        case ASTKind::LVALUE_EXPR:
            PrintAST(parser, root.lvalue.subscript, depth + 1);
            break;
        case ASTKind::CALL_EXPR:
            PrintASTList(root.call.args, depth + 1);
            break;

        case ASTKind::CONTINUE_STATEMENT:
        case ASTKind::BREAK_STATEMENT:
        case ASTKind::UNINITIALIZED:
            break;
        case ASTKind::COUNT: assert(0); break;
    }
}

static ASTIndex NewNode(Parser& parser, ASTKind kind) {
    parser.mem->emplace_back();
    parser.mem->back().kind = kind;
    return static_cast<ASTIndex>(parser.mem->size()-1);
}

static const Token& PeekCurrentToken(const Parser& parser) {
    assert(parser.tokenIdx < parser.tokens.size());
    return parser.tokens[parser.tokenIdx];
}
static const Token& PollCurrentToken(Parser& parser) {
    assert(parser.tokenIdx < parser.tokens.size());
    return parser.tokens[parser.tokenIdx++];
}

static AST& GetAST(const Parser& parser, ASTIndex idx) {
    assert(idx != AST_NULL);
    return (*parser.mem)[idx];
}

#define CompileErrorAt(parser, token, fmt) do { \
        CompileErrorMessage((parser).filename, (token).pos.line, (token).pos.col, fmt); \
        exit(1); \
    } while (0)


#define ExpectAndConsumeToken(parser, expectedKind, fmt) do { \
        const Token& token = PollCurrentToken(parser); \
        if (token.kind != expectedKind) \
            CompileErrorAt(parser, token, fmt); \
    } while (0)
#define ExpectAndConsumeTokens(parser, predicate, fmt) do { \
        const Token& token = PollCurrentToken(parser); \
        if (!predicate(token)) \
            CompileErrorAt(parser, token, fmt); \
    } while (0)


static ASTIndex NewNodeFromLastToken(Parser& parser, ASTKind kind) {
    ASTIndex idx = NewNode(parser, kind);
    AST& node = GetAST(parser, idx);
    node.tokenIdx = parser.tokenIdx - 1;
    return idx;
}


static ASTIndex ParseExpression(Parser& parser);
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

    const Token& openSubscript = PeekCurrentToken(parser);
    if (openSubscript.kind == TokenKind::LSQUARE) {
        ++parser.tokenIdx;
        ASTIndex arrSize = ParseExpression(parser);
        if (arrSize == AST_NULL)
#if 1
            ++parser.tokenIdx; // hack
#else
            CompileErrorAt(parser, PeekCurrentToken(parser), "Invalid subscript");
#endif
        ExpectAndConsumeToken(parser, TokenKind::RSQUARE, "Missing matching closing square bracket");
    }

    switch (typeTok.kind) {
        case TokenKind::I64: return { TypeKind::I64, 0 };
        case TokenKind::F64: return { TypeKind::F64, 0 };
        case TokenKind::U8:  return { TypeKind::U8 , 0 };
        default: assert(0 && "Unreachable"); break;
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

//     PrintToken(PeekCurrentToken(parser));
    fmt::print(stderr, "Parsed var defn: ");
    PrintToken(parser, GetAST(parser, defn).tokenIdx);
    fmt::print(stderr, "\n");
    return defn;
}

static ASTIndex ParseVarDefnAsgn(Parser& parser) {
    ASTIndex defn = ParseVarDefn(parser);
    if (defn == AST_NULL)
        return AST_NULL;

    const Token& maybeAsgn = PeekCurrentToken(parser);

    fmt::print(stderr, "maybeAsgn = ");
    PrintToken(maybeAsgn);
    fmt::print(stderr, "\n");
    if (maybeAsgn.kind == TokenKind::OPERATOR_ASSIGN) {
        // Assignment
        ++parser.tokenIdx;
        ASTIndex expr = ParseExpression(parser);
        if (expr == AST_NULL)
            CompileErrorAt(parser, maybeAsgn, "Expected expression after \"=\"");
        GetAST(parser, defn).defn.initExpr = expr;
    }

    return defn;
}

static ASTIndex ParseIf(Parser& parser) {
    return AST_NULL;
}

static ASTIndex ParseFor(Parser& parser) {
    const Token& token = PeekCurrentToken(parser);
    if (token.kind != TokenKind::FOR)
        return AST_NULL;

    // It is a for statement
    ++parser.tokenIdx;

    ExpectAndConsumeToken(parser, TokenKind::LPAREN, "Expected \"(\" after for statement");
    ASTIndex init = ParseVarDefnAsgn(parser);
    ExpectAndConsumeToken(parser, TokenKind::LPAREN, "Expected \";\" after for statement");

    return AST_NULL;
}

static ASTIndex ParseFactor(Parser& parser) {
    const Token& tok = PeekCurrentToken(parser);
    if (tok.kind == TokenKind::INTEGER_LITERAL) {

    }
    else if (tok.kind == TokenKind::FLOAT_LITERAL) {

    }
    else if (tok.kind == TokenKind::STRING_LITERAL) {

    }
    else if (tok.kind == TokenKind::CHAR_LITERAL) {

    }
    else if (tok.kind == TokenKind::OPERATOR_NEG) {

    }
    else if (tok.kind == TokenKind::OPERATOR_NOT) {

    }
    else if (tok.kind == TokenKind::LPAREN) {
        // Parenthesized expression

    }
    else if (tok.kind == TokenKind::IDENTIFIER) {
        // LValue or ProcedureCall

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
    return AST_NULL;
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
    return AST_NULL;
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
    return AST_NULL;
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
    return AST_NULL;
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
    return AST_NULL;
}

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
    return AST_NULL;
}

static ASTIndex ParseAssignment(Parser& parser) {
    return AST_NULL;
}
static ASTList NewASTList(Parser& parser) {
    parser.indices.emplace_back();
    return static_cast<ASTList>(parser.indices.size() - 1);
}

static void AddToASTList(Parser& parser, ASTList list, ASTIndex idx) {
    parser.indices[list].push_back(idx);
}


static ASTIndex ParseStatement(Parser& parser) {
    // TODO: implement this

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
        ASTIndex expr = ParseExpression(parser);
        GetAST(parser, ret).ret.expr = expr;
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
        if (assign != AST_NULL)
            return assign;
    }

    // If all else failed, try parse ParseExpression
    ASTIndex expr = ParseExpression(parser);
    if (expr == AST_NULL) {
        PrintToken(PollCurrentToken(parser));
        fmt::print(stderr, "\n");
    }
    assert(expr != AST_NULL);
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
            if (stmt == AST_NULL)
                CompileErrorAt(parser, nextTok, "SOMETHING WENT WRONG");

            AddToASTList(parser, body, stmt);
        }
    }
    else {
        // Parse single statement
        ASTIndex stmt = ParseStatement(parser);
        if (stmt == AST_NULL)
            CompileErrorAt(parser, PeekCurrentToken(parser), "SOMETHING WENT WRONG");

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
    while (parser.tokenIdx < parser.tokens.size()) {
        ASTIndex proc = ParseProcedure(parser);

        const AST& root = GetAST(parser, proc);
        fmt::print(stderr, "{}\n", ASTKindName(root.kind));
        if (root.tokenIdx != 0) {
            PrintToken(parser, root.tokenIdx);
            fmt::print(stderr, "\n");
        }
        for (ASTIndex paramIdx : parser.indices[root.proc.params]) {
            fmt::print(stderr, "\t");

            const AST& param = GetAST(parser, paramIdx);
            fmt::print(stderr, "{} {} {}\n",
                (param.defn.isConst ? "LET" : "MUT"),
                (param.defn.type == TypeKind::I64 ? "I64" :
                 param.defn.type == TypeKind::F64 ? "F64" :
                 param.defn.type == TypeKind::U8 ? "U8" : "NONE"),
                parser.tokens[param.tokenIdx].text);
        }
        fmt::print(stderr, "\t-> ");
        if (root.proc.retType != TypeKind::NONE) {
            fmt::print(stderr, "{}\n", (root.proc.retType == TypeKind::I64 ? "I64" :
                root.proc.retType == TypeKind::F64 ? "F64" :
                root.proc.retType == TypeKind::U8 ? "U8" : "SOMETHING WENT WRONG"));
        }
        else {
            fmt::print(stderr, "NONE\n");
        }
        fmt::print(stderr, "\n");

        return;

        if (proc == AST_NULL) {
            return;
        }
    }
}

#define STRINGIFY(x) #x
#define PRINT_SIZE(x) fmt::print("{}: {} {}\n", STRINGIFY(x), sizeof(x), alignof(x))
std::vector<AST> ParseEntireProgram(Parser& parser) {
    PRINT_SIZE(AST::ASTProgram);
    PRINT_SIZE(AST::ASTProcedure);
    PRINT_SIZE(AST::ASTBinaryOperator);
    PRINT_SIZE(AST::ASTUnaryOperator);
    PRINT_SIZE(AST::ASTLiteral);
    PRINT_SIZE(AST::ASTLValue);
    PRINT_SIZE(AST::ASTCall);
    PRINT_SIZE(AST::ASTIf);
    PRINT_SIZE(AST::ASTFor);
    PRINT_SIZE(AST::ASTDefinition);
    PRINT_SIZE(AST::ASTAssign);
    PRINT_SIZE(AST::ASTReturn);
    PRINT_SIZE(AST);
    return {};
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

