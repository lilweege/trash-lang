#include "analyzer.hpp"
#include "compileerror.hpp"
#include "parser.hpp"

#include <unordered_map>

class Analyzer {
    struct ProcedureDefn {
        std::vector<ASTIndex> paramTypes;
        TypeKind returnType; // Optional
    };

    const File file;
    const std::vector<Token>& tokens;
    AST& ast;
    std::unordered_map<std::string_view, ProcedureDefn> procedureDefns;

    void AssertIdentUnusedInCurrentScope(
        const std::unordered_map<std::string_view, ASTIndex>& symbolTable,
        const Token& ident);
    void VerifyProcedure(ASTIndex procIdx);

public:
    Analyzer(File file_, const std::vector<Token>& tokens_, AST& ast_)
        : file{file_}, tokens{tokens_}, ast{ast_}
    {}

    void VerifyProgram();
};

void Analyzer::AssertIdentUnusedInCurrentScope(
    const std::unordered_map<std::string_view, ASTIndex>& symbolTable,
    const Token& ident)
{
    if (procedureDefns.contains(ident.text)) {
        // Variable name same as function name, not allowed
        CompileErrorAt(ident, "Variable \"{}\" shadows function", ident.text);
    }
    else if (symbolTable.contains(ident.text)) {
        // Impossible unless global variable (not implemented)
        CompileErrorAt(ident, "Redefinition of variable \"{}\"", ident.text);
    }
}

void Analyzer::VerifyProcedure(ASTIndex procIdx) {
    ASTNode::ASTProcedure procedure = ast.tree[procIdx].procedure;

    ASTList params = procedure.params;
    std::unordered_map<std::string_view, ASTIndex> symbolTable; // TODO: use more efficient structure
    // When entering and leaving lexical scopes, the entire table needs to be copied
    // This is probably not a good idea, but I can't really think of anything better right now
    // Maybe some sort of linked structure (constant insert/remove)?
    if (params != AST_EMPTY) {
        for (ASTIndex param : ast.lists[params]) {
            // Add param to scope
            const Token& ident = tokens[ast.tree[param].tokenIdx];
            AssertIdentUnusedInCurrentScope(symbolTable, ident);
            symbolTable[ident.text] = param;
        }
    }

    // TODO: Recurse
    // Typecheck all expressions
    // Check all varialbe names in lexical scopes (undefined, redefinition)
    // TODO: list of builtin functions
    ASTList body = procedure.body;
    if (body != AST_EMPTY) {
        for (ASTIndex stmtIdx : ast.lists[body]) {
            const ASTNode& statement = ast.tree[stmtIdx];
            switch (statement.kind) {
                case ASTKind::IF_STATEMENT: { } break;
                case ASTKind::FOR_STATEMENT: { } break;
                case ASTKind::DEFINITION: { } break;
                case ASTKind::RETURN_STATEMENT: { } break;
                case ASTKind::BREAK_STATEMENT: { } break;
                case ASTKind::CONTINUE_STATEMENT: { } break;
                case ASTKind::ASSIGN: { } break;
                case ASTKind::CALL_EXPR: { } break;
                // TODO: ... other kinds of expressions
                default: assert(0 && "Unimplemented!"); break;
            }
        }
    }

}

void Analyzer::VerifyProgram() {
    ASTNode& prog = ast.tree[0];

    // Collect all procedure definitions and "forward declare" them.
    // Mutual recursion should work out of the box
    auto& procedures = ast.lists[prog.program.procedures];
    for (ASTIndex procIdx : procedures) {
        ASTNode& proc = ast.tree[procIdx];
        const Token& procName = tokens[proc.tokenIdx];
        if (procedureDefns.contains(procName.text))
            CompileErrorAt(procName, "Redefinition of procedure \"{}\"", procName.text);

        auto& params = ast.lists[proc.procedure.params];
        procedureDefns.emplace(procName.text, ProcedureDefn{
//                 .paramTypes = std::move(params), // is this a bug?
                .paramTypes = params, // is this a bug?
                .returnType = proc.procedure.retType,
            });
    }

    for (ASTIndex procIdx : procedures) {
        VerifyProcedure(procIdx);
    }
}

void VerifyAST(File file, const std::vector<Token>& tokens, AST& ast) {
    Analyzer analyzer{file, tokens, ast};
    analyzer.VerifyProgram();
}
