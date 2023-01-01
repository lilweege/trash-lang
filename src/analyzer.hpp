#pragma once

#include "parser.hpp"

#include <unordered_map>

struct Type {
    TypeKind kind;
    bool isScalar;
};

class Analyzer {
    struct ProcedureDefn {
        std::vector<ASTIndex> paramTypes;
        TypeKind returnType; // Optional
    };

    const File file;
    const std::vector<Token>& tokens;
    AST& ast;
    std::unordered_map<std::string_view, ProcedureDefn> procedureDefns;

    TypeKind currProcRetType;
    bool returnAtTopLevel;
    int loopDepth{};
    int blockDepth{};

    void AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident);
    void VerifyProcedure(ASTIndex procIdx);
    void VerifyStatements(ASTList list, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyStatement(ASTIndex stmtIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyDefinition(ASTIndex defnIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyCall(ASTIndex callIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
public:
    Analyzer(File file_, const std::vector<Token>& tokens_, AST& ast_)
        : file{file_}, tokens{tokens_}, ast{ast_}
    {}

    void VerifyProgram();
};

void VerifyAST(File file, const std::vector<Token>& tokens, AST& ast);
