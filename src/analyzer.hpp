#pragma once

#include "parser.hpp"

#include "bytecode.hpp"
#include <unordered_map>

struct Type {
    std::string_view typeName; //TypeKind kind;
    uint32_t numPointerLevels; // bool isScalar;

    auto operator<=>(const Type&) const = default;
};

struct Procedure {
    std::vector<Instruction> instructions;
    ASTNode::ASTProcedure procInfo;
    std::string_view procName;
    size_t insStartIdx, insEndIdx;
    std::vector<ASTNode::ASTDefinition> params;
};

class Analyzer {
    struct TypeDefn {
        enum class StorageKind {
            U8,  // Pass by value (general register)
            I64, // Pass by value (general register)
            // F32, // Pass by value (wide register)
            F64, // Pass by value (wide register)
            STRUCT, // Pass by pointer (general register)
        };

        std::vector<std::pair<Type, size_t>> memberOffsets;
        size_t size;
        size_t alignment;
        StorageKind storageKind;
    };

    struct ProcedureDefn {
        std::vector<Type> paramTypes;
        Type returnType;
        bool hasReturnType;
        size_t instructionNum;
    };

    const std::vector<Token>& tokens;
    AST& ast;
    std::unordered_map<std::string_view, ProcedureDefn> procedureDefns;
    std::unordered_map<std::string_view, TypeDefn> resolvedTypes;
    std::vector<std::pair<size_t, std::string_view>> unresolvedCalls;

    ProcedureDefn* currProc;
    int loopDepth{};
    int blockDepth{};
    bool returnAtTopLevel;
    size_t maxStaticStackSize; // size_t maxNumVariables;
    std::vector<std::vector<size_t>> breakAddrs;
    std::vector<std::vector<size_t>> continueAddrs;
    std::vector<std::unordered_map<std::string_view, size_t>> stackAddrs;
    std::vector<size_t> blockStackSize;
    bool keepGenerating = true;
    std::vector<Instruction> instructions;

    void AssertIdentUnusedInCurrentScope(const std::unordered_map<std::string_view, ASTIndex>& symbolTable, const Token& ident);
    Type VerifyType(ASTIndex typeIdx);
    uint32_t TypeSize(Type type);
    uint32_t TypeAlign(Type type);
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
