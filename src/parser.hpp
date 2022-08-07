#pragma once

#include "tokenizer.hpp"

#include <vector>

// Language Grammar
// TODO: bitwise operators, pointers, continue, break, struct

// Program             =  { Procedure }
// ProcedureDefn       =  "proc" Identifier "(" [ VariableDefn { "," VariableDefn } ] ")" [ "->" Type ] Body

// Body                =  Block | Statement
// Block               =  "{" { Statement } "}"
// Statement           =  IfStmt | ForStmt | ( VariableDefnAsgn | Assignment | Expression | "return" Expression | "break" | "continue" ) ";"

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
    // UNINITIALIZED,
    PROGRAM,
    PROCEDURE,
    BODY,
    IF_STATEMENT,
    FOR_STATEMENT,
    RETURN_STATEMENT,
    CONTINUE_STATEMENT,
    BREAK_STATEMENT,
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

enum class TypeKind : uint8_t {
    NONE, // void functions
    U8,
    I64,
    F64,
};


using TokenIndex = uint32_t;
using ASTIndex = uint32_t;


// TODO: change all of this
struct AST {
    // std::stringview not trivial?
//     using StringView = std::string_view;
    struct StringView {
        const char* buf;
        size_t sz;
    };


    // TODO: remove redundant firstIndex when it will always immediately follow
    struct ASTList {
        ASTIndex firstStmt;
        uint32_t numStmts;
    };

    struct ASTProcedure {
        ASTList params;
        ASTList body;
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
            StringView str;
        };
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
        ASTIndex size; // Should be integer literal
        ASTIndex initExpr; // Optional
        bool isConst; // let/mut
        TypeKind type;
    };

    struct ASTAssign {
        // Access ident through token
        ASTIndex subscript; // Optional
        ASTIndex expession;
    };

    struct ASTReturn {
        ASTIndex expr;
    };

    ASTKind kind;
//     union {
//         // TODO: flags ...
//     };
    TokenIndex tokenIdx; // Optional. Used for identifiers
    // 8 bytes

    union {
        ASTProcedure proc;

        ASTBinaryOperator binaryOp;
        ASTUnaryOperator unaryOp;
        ASTLiteral literal;

        ASTIf ifstmt;
        ASTFor forstmt;
        ASTDefinition defn;
        ASTAssign asgn;

        ASTReturn ret;
    };
};


struct Parser {
    std::string_view filename;
    std::vector<Token> tokens;
    size_t tokenIdx;
};

std::vector<AST> ParseProgram(Parser& parser);


// static_assert(std::is_trivial_v<AST>);

