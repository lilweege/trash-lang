#pragma once

#include "parser.hpp"

#include "bytecode.hpp"
#include <unordered_map>

struct Type {
    TypeKind kind;
    bool isScalar;
};

class Analyzer {
    struct ProcedureDefn {
        std::vector<ASTIndex> paramTypes;
        TypeKind returnType; // Optional
        size_t stackSpace;
        size_t instructionNum;
    };

    const File file;
    const std::vector<Token>& tokens;
    AST& ast;
    std::unordered_map<std::string_view, ProcedureDefn> procedureDefns;
    std::vector<std::pair<size_t, std::string_view>> unresolvedCalls;

    ProcedureDefn* currProc;
    int entryAddr;
    int loopDepth{};
    int blockDepth{};
    bool hasEntry{};
    bool returnAtTopLevel;
    size_t maxNumVariables;
    std::vector<std::vector<size_t>> breakAddrs;
    std::vector<std::vector<size_t>> continueAddrs;
    std::vector<std::unordered_map<std::string_view, size_t>> stackAddrs;
    bool keepGenerating = true;

    void AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident);
    void VerifyProcedure(ASTIndex procIdx);
    void VerifyStatements(ASTList list, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyStatement(ASTIndex stmtIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    void VerifyDefinition(ASTIndex defnIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyLValue(ASTIndex lvalIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable, bool isLoading);
    Type VerifyCall(ASTIndex callIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);
    Type VerifyExpression(ASTIndex exprIdx, std::unordered_map<std::string_view, ASTIndex>& symbolTable);

    std::vector<Instruction> instructions;
    void AddInstruction(Instruction ins);
public:
    Analyzer(File file_, const std::vector<Token>& tokens_, AST& ast_)
        : file{file_}, tokens{tokens_}, ast{ast_}
    {}

    void VerifyProgram();
};

void VerifyAST(File file, const std::vector<Token>& tokens, AST& ast);
