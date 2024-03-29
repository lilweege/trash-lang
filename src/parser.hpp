#pragma once

#include "compileerror.hpp"
#include "tokenizer.hpp"

#include <vector>
#include <stdint.h>

// Language Grammar
// TODO: bitwise operators, pointers, struct

// Program             =  { Procedure }
// Procedure           =  "proc" Identifier "(" [ VariableDefn { "," VariableDefn } ] ")" [ "->" Type ] Body

// Body                =  Block | Statement
// Block               =  "{" { Statement } "}"
// Statement           =  IfStmt | ForStmt | ( VariableDefnAsgn | Assignment | Expression | "return" [ Expression ] | "break" | "continue" ) ";"

// IfStmt              =  "if" "(" Expression ")" Body [ "else" Body ]
// ForStmt             =  "for" "(" [ VariableDefnAsgn ] ";" [ Expression ] ";" [ Assignment ] ")" Body

// Expression          =  LogicalTerm { "||" LogicalTerm }
// LogicalTerm         =  LogicalFactor { "&&" LogicalFactor }
// LogicalFactor       =  Comparison { ( "==" | "!=" ) Comparison }
// Comparison          =  Sum { ( ">=" | "<=" | ">" | "<" ) Sum }
// Sum                 =  Term { ( "+" | "-" ) Term }
// Term                =  Factor { ( "*" | "/" | "%" ) Factor }
// Factor              =  IntegerLiteral | FloatLiteral | StringLiteral | CharacterLiteral | ProcedureCall | LValue | "(" Expression ")" | "-" Factor | "!" Factor
// ProcedureCall       =  Identifier "(" [ Expression { "," Expression } ] ")"

// Assignment          =  LValue "=" Expression
// VariableDefnAsgn    =  VariableDefn [ "=" Expression ]
// VariableDefn        =  ( "let" | "mut" ) Type Identifier
// Type                =  Primitive [ Subscript ]
// LValue              =  Identifier [ Subscript ]
// Subscript           =  "[" Expression "]"
// Primitive           =  "u8" | "i64" | "f64"

// Technically not EBNF...
// LineComment      =  ? ...
// IntegerLiteral   =  [0-9]+
// FloatLiteral     =  IntegerLiteral . IntegerLiteral
// StringLiteral    =  " { AsciiOrEscapeChar } "
// CharacterLiteral =  ' AsciiOrEscapeChar '

enum class ASTKind : uint8_t {
    UNINITIALIZED,
    PROGRAM,
    PROCEDURE,
    IF_STATEMENT,
    FOR_STATEMENT,
    RETURN_STATEMENT,
    CONTINUE_STATEMENT,
    BREAK_STATEMENT,
    ASM_STATEMENT,
    DEFINITION,
    ASSIGN,
    NEG_UNARYOP_EXPR,
    NOT_UNARYOP_EXPR,
    EQ_BINARYOP_EXPR,
    NE_BINARYOP_EXPR,
    GE_BINARYOP_EXPR,
    GT_BINARYOP_EXPR,
    LE_BINARYOP_EXPR,
    LT_BINARYOP_EXPR,
    AND_BINARYOP_EXPR,
    OR_BINARYOP_EXPR,
    ADD_BINARYOP_EXPR,
    SUB_BINARYOP_EXPR,
    MUL_BINARYOP_EXPR,
    DIV_BINARYOP_EXPR,
    MOD_BINARYOP_EXPR,
    INTEGER_LITERAL_EXPR,
    FLOAT_LITERAL_EXPR,
    CHAR_LITERAL_EXPR,
    STRING_LITERAL_EXPR,
    LVALUE_EXPR,
    CALL_EXPR,

    // ...
    COUNT
};
const char* ASTKindName(ASTKind kind);

enum class TypeKind : uint8_t {
    NONE, // void functions
    U8,
    I64,
    F64,
    STR,
    // ...
    COUNT
};
const char* TypeKindName(TypeKind kind);

// TODO: strong types
using TokenIndex = uint32_t;
using ASTIndex = uint32_t;
using ASTList = uint32_t; // index into vector of vector of ASTList
const TokenIndex TOKEN_NULL = 0;
const ASTIndex AST_NULL = 0; // Null points to the root
const ASTList AST_EMPTY = 0;


struct ASTNode {
//     using StringView = std::string_view; // std::string_view is not trivial :(
    struct StringView {
        const char* buf;
        size_t sz;
    };

    struct ASTProgram {
        ASTList procedures;
    };

    struct ASTProcedure {
        ASTList params;
        ASTList body;
        TypeKind retType;
        bool retIsArray;
        bool isCdecl;
        bool isExtern;
        bool isPublic;
    };

    struct ASTUnaryOperator {
        ASTIndex expr;
    };

    struct ASTBinaryOperator {
        ASTIndex left;
        ASTIndex right;
    };

    struct ASTLiteral {
        union {
            uint64_t i64;
            double f64;
            uint8_t u8;
            StringView str; // largest member
        };
    };

    struct ASTLValue {
        // Access ident through token
        ASTIndex subscript; // Optional
    };

    struct ASTCall {
        // Access proc name through token
        ASTList args;
    };

    struct ASTIf {
        ASTIndex cond;
        ASTList body;
        ASTList elsestmt; // Optional
    };

    struct ASTFor {
        ASTIndex init;
        ASTIndex cond;
        ASTIndex incr;
        ASTList body;
    };

    struct ASTDefinition {
        // Access ident through token
        ASTIndex arraySize;
        ASTIndex initExpr; // Optional
        // These variables should move to flags but the other structs in the union are bigger anyways
        bool isConst; // let/mut
        TypeKind type;
    };

    struct ASTAssign {
        // Access ident through token
        ASTIndex lvalue;
        ASTIndex rvalue;
    };

    struct ASTReturn {
        ASTIndex expr;
    };

    struct ASTAsm {
        ASTList strings;
    };

    union {
        ASTProgram program;
        ASTProcedure procedure;
        ASTBinaryOperator binaryOp;
        ASTUnaryOperator unaryOp;
        ASTLiteral literal;
        ASTLValue lvalue;
        ASTCall call;
        ASTIf ifstmt;
        ASTFor forstmt;
        ASTDefinition defn;
        ASTAssign asgn;
        ASTReturn ret;
        ASTAsm asm_;
    };

    // String literal is making the struct big, so these aren't useful
    // If this struct becomes smaller, this would be helpful.
    // Currently this space is just wasted padding anyways...
//     uint8_t flags1;
//     uint8_t flags2;
//     uint8_t flags3;
//     union {
//         // TODO: flags ...
//     };
    TokenIndex tokenIdx; // Optional. Used for identifiers
    ASTKind kind;
};

struct AST {
    std::vector<ASTNode> tree;
    std::vector<std::vector<ASTIndex>> lists;
};

class Parser {
    const std::vector<Token>& tokens;
    TokenIndex tokenIdx{};
    AST& ast;

    ASTIndex NewNode(ASTKind kind);
    ASTIndex NewNodeFromToken(TokenIndex tokIdx, ASTKind kind);
    ASTIndex NewNodeFromLastToken(ASTKind kind);
    [[nodiscard]] const Token& PeekCurrentToken() const;
    const Token& PollCurrentToken();
    ASTList NewASTList();
    void AddToASTList(ASTList list, ASTIndex idx);
    ASTIndex ParseSubscript();
    std::pair<TypeKind, ASTIndex> ParseType();
    ASTIndex ParseVarDefn();
    ASTIndex ParseVarDefnAsgn();
    ASTIndex ParseIf();
    ASTIndex ParseCall();
    ASTIndex ParseLValue();
    ASTIndex ParseAssignment();
    ASTIndex ParseFor();
    ASTIndex ParseFactor();
    ASTIndex ParseTerm();
    ASTIndex ParseValue();
    ASTIndex ParseComparison();
    ASTIndex ParseLogicalFactor();
    ASTIndex ParseLogicalTerm();
    ASTIndex ParseExpression();
    ASTIndex ParseStatement();
    ASTList ParseBody();
    ASTIndex ParseProcedure();
    void ParseProgram();


public:
    Parser(const std::vector<Token>& tokens_, AST& ast_)
        : tokens{tokens_}, ast{ast_}
    {}

    void ParseEntireProgram();
    void PrintNode(ASTIndex rootIdx) const;
    void PrintAST(ASTIndex rootIdx, uint32_t depth) const;
};

AST ParseEntireProgram(const std::vector<Token>& tokens);

static_assert(std::is_trivial_v<ASTNode>);
static_assert(sizeof(ASTNode) == 24); // Ensure this struct doesn't accidentally get bigger

