#pragma once

#include "parser.hpp"

#include "bytecode.hpp"
#include <unordered_map>

struct Type {
    TypeKind kind;
    bool isScalar;
};

struct Procedure {
    std::vector<Instruction> instructions;
    ASTNode::ASTProcedure procInfo;
    std::string_view procName;
    size_t insStartIdx, insEndIdx;
    std::vector<ASTNode::ASTDefinition> params;
};

class Analyzer {
    struct ProcedureDefn {
        std::vector<ASTIndex> paramTypes;
        Type returnType;
        size_t stackSpace;
        size_t instructionNum;
    };

    const std::vector<Token>& tokens;
    AST& ast;
    std::unordered_map<std::string_view, ProcedureDefn> procedureDefns;
    std::vector<std::pair<size_t, std::string_view>> unresolvedCalls;

    ProcedureDefn* currProc;
    // int entryAddr;
    int loopDepth{};
    int blockDepth{};
    // bool hasEntry{};
    bool returnAtTopLevel;
    size_t maxNumVariables;
    std::vector<std::vector<size_t>> breakAddrs;
    std::vector<std::vector<size_t>> continueAddrs;
    std::vector<std::unordered_map<std::string_view, size_t>> stackAddrs;
    bool keepGenerating = true;
    std::vector<Instruction> instructions;

    void AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident);
    void VerifyProcedure(ASTIndex procIdx);
    void VerifyStatements(ASTList list, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyStatement(ASTIndex stmtIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyDefinition(ASTIndex defnIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyLValue(ASTIndex lvalIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable, bool isLoading);
    Type VerifyCall(ASTIndex callIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);

    void AddInstruction(Instruction ins);
public:
    std::vector<Procedure> procedures;

    Analyzer(const std::vector<Token>& tokens_, AST& ast_)
        : tokens{tokens_}, ast{ast_}
    {}

    void VerifyProgram();
};

std::vector<Procedure> VerifyAST(const std::vector<Token>& tokens, AST& ast);
