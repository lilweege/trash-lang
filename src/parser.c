#include "compiler.h"
#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

const char* nodeKindName(NodeKind kind) {
    static_assert(NODE_COUNT == 32, "Exhaustive check of node kinds failed");
    const char* NodeKindNames[32] = {
        "NODE_PROGRAM",
        "NODE_IF",
        "NODE_ELSE",
        "NODE_WHILE",
        "NODE_BLOCK",
        "NODE_STATEMENT",
        "NODE_CALL",
        "NODE_PROCEDURE",
        "NODE_DEFINITION",
        "NODE_ARGUMENT",
        "NODE_TYPE",
        "NODE_INTEGER",
        "NODE_FLOAT",
        "NODE_STRING",
        "NODE_CHAR",
        "NODE_ASSIGN",
        "NODE_LVALUE",
        "NODE_EQ",
        "NODE_NE",
        "NODE_GE",
        "NODE_GT",
        "NODE_LE",
        "NODE_LT",
        "NODE_NOT",
        "NODE_AND",
        "NODE_OR",
        "NODE_ADD",
        "NODE_SUB",
        "NODE_MUL",
        "NODE_DIV",
        "NODE_MOD",
        "NODE_NEG",
    };
    return NodeKindNames[kind];
}

#define SCRATCH_SIZE 8192
AST scratchBuffer[SCRATCH_SIZE];
AST* newNode(NodeKind kind, Token token) {
    // TODO: perhaps store these somewhere local
    // AST* node = (AST*) malloc(sizeof(AST)); // assume this malloc never fails for now
    static int sz = 0;
    assert(sz < SCRATCH_SIZE-1);
    AST* node = scratchBuffer + sz++;
    node->kind = kind;
    node->token = token;
    node->left = node->right = NULL;
    return node;
}


void printAST(AST* root, size_t depth) {
    if (root == NULL) return;
    for (size_t i = 0; i < depth; ++i) {
        putchar('\t');
    }
    printf("[%s]", nodeKindName(root->kind));
    if (root->kind == NODE_INTEGER || root->kind == NODE_FLOAT || root->kind == NODE_STRING || root->kind == NODE_CHAR || root->kind == NODE_DEFINITION || root->kind == NODE_TYPE || (root->kind == NODE_STATEMENT && root->token.kind != TOKEN_NONE) || root->kind == NODE_LVALUE || root->kind == NODE_CALL /* || ...*/) {
        printf(": <%s: \""SV_FMT"\">", tokenKindName(root->token.kind), SV_ARG(root->token.text));
    }
    printf("\n");
    size_t newDepth = depth + (root->kind == NODE_STATEMENT || root->kind == NODE_ARGUMENT ? 0 : 1);
    printAST(root->left, newDepth);
    printAST(root->right, newDepth);
}

// TODO: fix leaks

AST* parseProgram(Tokenizer* tokenizer) {
    pollToken(tokenizer);
    AST* resultTree = newNode(NODE_PROGRAM, (Token){0});
    resultTree->right = newNode(NODE_STATEMENT, (Token){0});
    AST* curr = resultTree->right;

    while (1) {
        AST* statement = parseProcedure(tokenizer);
        if (statement == NULL) {
            statement = parseStatement(tokenizer);
        }
        
        // AST* statement = parseStatement(tokenizer);
        if (statement == NULL) {
            break;
        }
        assert(statement->kind != NODE_STATEMENT);
        curr->right = newNode(NODE_STATEMENT, (Token){0});
        curr->left = statement;
        curr = curr->right;
    }
    printAST(resultTree, 0);
    return resultTree;
}

// procedure       ->  type identifier ( [ type identifier { , type identifier } ] ) block
AST* parseProcedure(Tokenizer* original) {
    // return NULL;
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;

    printf("PARSING PROC\n");
    AST* type = parseType(tokenizer);
    if (type == NULL) return NULL;
    printf("PARSING PROC: GOT TYPE\n");
    if (!pollToken(tokenizer)) {
        return NULL;
        // compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
        //              "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) {
        return NULL;
    }
    Token id = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;
    printf("PARSING PROC: GOT NAME\n");
    if (!pollToken(tokenizer)) {
        return NULL;
    }
    if (tokenizer->nextToken.kind != TOKEN_LPAREN) {
        return NULL;
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    // it is a procedure
    AST* proc = newNode(NODE_PROCEDURE, id);
    
    // TODO
    printf("======\t\tPARSE PROC\t\t======\n");
    printf("[%s]: <%s: \""SV_FMT"\">\n", nodeKindName(proc->kind), tokenKindName(proc->token.kind), SV_ARG(proc->token.text));

    AST* firstParam = parseDefinition(tokenizer);

    if (firstParam == NULL) {
        assert(pollToken(tokenizer));
        if (tokenizer->nextToken.kind == TOKEN_RPAREN) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            proc->right = parseBlock(tokenizer);
            *original = current;
            printf("DONE PARSING BLOCK\n");
            return proc;
        }
        else {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Invalid parameter");
        }
    }
    printf("PROC: FIRST PARAM\n");

    //         Procedure
    //         /      |
    //     Defn    Block
    //     / |
    // Type  Defn
    //     / |
    // Type  Defn
    //    ...

    proc->left = firstParam;
    AST* curr = proc->left;

    // curr->right = newNode(NODE_ARGUMENT, (Token){0});
    // curr->left = firstArg;
    // curr = curr->right;

    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, proc->token.pos },
                        "Unmatched token");
        }
        if (tokenizer->nextToken.kind == TOKEN_RPAREN) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            break;
        }
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_COMMA) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            curr->right = parseDefinition(tokenizer);
            if (curr->right == NULL) {
                compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                            "Invalid argument");
            }
            curr = curr->right;
        }
        else {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Expected comma separator");
        }
    }
    
    proc->right = parseBlock(tokenizer);
    *original = current;
    return proc;
}

#define expectSemicolon(tokenizer, token) do { \
    if (!pollToken(tokenizer) || tokenizer->nextToken.kind != TOKEN_SEMICOLON) { \
        compileError((FileInfo) { tokenizer->filename, token.pos }, \
                     "Expected semicolon after token"); \
    } \
    tokenizer->nextToken.kind = TOKEN_NONE; \
} while (0)

AST* parseStatement(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) return NULL;
    AST* result = parseBranch(tokenizer);
    if (result == NULL) {
        result = parseDefinition(tokenizer);
        if (result != NULL) {
            expectSemicolon(tokenizer, result->token);
        }
    }
    if (result == NULL) {
        result = parseAssignment(tokenizer);
    }
    if (result == NULL) {
        result = parseExpression(tokenizer);
        if (result == NULL) {
            // TODO: good error message
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                         "Unexpected token");
            return NULL;
        }
        expectSemicolon(tokenizer, result->token);
    }
    assert(result != NULL);
    return result;
}

AST* parseBranch(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind != TOKEN_IF && 
            tokenizer->nextToken.kind != TOKEN_ELSE && 
            tokenizer->nextToken.kind != TOKEN_WHILE) {
        return NULL;
    }
    NodeKind branchKind =
        tokenizer->nextToken.kind == TOKEN_IF ? NODE_IF :
        tokenizer->nextToken.kind == TOKEN_WHILE ? NODE_WHILE : NODE_ELSE;

    if (branchKind == NODE_ELSE) {
        // else should already be handled and skipped
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "`else` without previous `if`");
    }

    AST* branch = newNode(branchKind, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LPAREN) {
        compileError((FileInfo) { tokenizer->filename, branch->token.pos },
                     "Expected ( after token");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    branch->left = parseExpression(tokenizer);
    if (branch->left == NULL) {
        compileError((FileInfo) { tokenizer->filename, branch->token.pos },
                     "Invalid expression after (");
    }

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
        compileError((FileInfo) { tokenizer->filename, branch->token.pos },
                     "Expected ) after expression");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    

#define tryGetBody(node) do {                                                           \
        (node)->right = parseBlock(tokenizer);                                          \
        if ((node)->right == NULL) {                                                    \
            (node)->right = newNode(NODE_STATEMENT, (Token){0});                        \
            (node)->right->left = parseStatement(tokenizer);                            \
        }                                                                               \
        if ((node)->right == NULL) {                                                    \
            compileError((FileInfo) { tokenizer->filename, (node)->token.pos },         \
                        "Expected conditional body");                                   \
        }                                                                               \
    } while (0)


    tryGetBody(branch);

    if (branchKind == NODE_IF) {
        // try to get matching else
        // defer condition to else->left so that if->left can point to eles
        if (!pollToken(tokenizer)) return branch;
        if (tokenizer->nextToken.kind == TOKEN_ELSE) {
            // found matching else
            tokenizer->nextToken.kind = TOKEN_NONE;
            // modify the tree so if/else can be matched easily
            // if `if` has corresponding `else`:
                // if->left points to else
                // defer condition to else->left (which would have been NULL otherwise)
            AST* branchElse = newNode(NODE_ELSE, tokenizer->nextToken);
            AST* condition = branch->left;
            branch->left = branchElse;
            branchElse->left = condition; // deferred condition
            tryGetBody(branchElse);
        }
    }

    return branch;
#undef tryGetBody
}


AST* parseBlock(Tokenizer* tokenizer) {
    //    B
    //    |
    //   S0
    //  / |
    // A S1
    //  / |
    // B S2
    //  / |
    // C S3
    // ...

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }

    if (tokenizer->nextToken.kind != TOKEN_LCURLY) return NULL;
    AST* block = newNode(NODE_BLOCK, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    block->right = newNode(NODE_STATEMENT, (Token){0});
    AST* curr = block->right;

    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, block->token.pos },
                        "Unmatched token");
        }
        if (tokenizer->nextToken.kind == TOKEN_RCURLY) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            break;
        }
        curr->right = newNode(NODE_STATEMENT, (Token){0});
        curr->left = parseStatement(tokenizer);
        curr = curr->right;
    }
    return block;
}

AST* parseDefinition(Tokenizer* tokenizer) {
    AST* type = parseType(tokenizer);
    if (type == NULL) return NULL;
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Expected identifier after type");
    }
    AST* defn = newNode(NODE_DEFINITION, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    // expectSemicolon(tokenizer, defn->token);
    defn->left = type;
    return defn;
}

AST* parseAssignment(Tokenizer* original) {
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;
    
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    AST* lvalue = parseLvalue(tokenizer);
    if (lvalue == NULL) return NULL;

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_OPERATOR_ASSIGN) return NULL;
    Token eq = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    AST* expr = parseExpression(tokenizer);
    if (expr == NULL) {
        // TODO: better error message
        compileError((FileInfo) { tokenizer->filename, eq.pos },
                     "Invalid assignment");
    }
    expectSemicolon(tokenizer, expr->token);
    
    AST* result = newNode(NODE_ASSIGN, eq);
    result->left = lvalue;
    result->right = expr;
    *original = current;
    return result;
}

AST* parseExpression(Tokenizer* tokenizer) {
    AST* root = parseLogicalTerm(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_OR) {
            break;
        }
        AST* result = newNode(NODE_OR, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseLogicalTerm(tokenizer);
        curr = result;
    }
    return curr;
}

/* start of uninteresting functions */
// these are all essentially the same as parseExpression
// see grammar definition in parser.h to see why

AST* parseLogicalTerm(Tokenizer* tokenizer) {
    AST* root = parseLogicalFactor(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_AND) {
            break;
        }
        AST* result = newNode(NODE_AND, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseLogicalFactor(tokenizer);
        curr = result;
    }
    return curr;
}

AST* parseLogicalFactor(Tokenizer* tokenizer) {
    AST* root = parseComparison(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_EQ &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_NE) {
            break;
        }
        NodeKind resultKind =
            tokenizer->nextToken.kind == TOKEN_OPERATOR_EQ ? NODE_EQ : NODE_NE;
        AST* result = newNode(resultKind, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseComparison(tokenizer);
        curr = result;
    }
    return curr;
}

AST* parseComparison(Tokenizer* tokenizer) {
    AST* root = parseValue(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_GE &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_LE &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_GT &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_LT) {
            break;
        }
        NodeKind resultKind =
            tokenizer->nextToken.kind == TOKEN_OPERATOR_GE ? NODE_GE :
            tokenizer->nextToken.kind == TOKEN_OPERATOR_LE ? NODE_LE :
            tokenizer->nextToken.kind == TOKEN_OPERATOR_GT ? NODE_GT : NODE_LT;
        AST* result = newNode(resultKind, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseValue(tokenizer);
        curr = result;
    }
    return curr;
}

AST* parseValue(Tokenizer* tokenizer) {
    AST* root = parseTerm(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_POS &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_NEG) {
            break;
        }
        NodeKind resultKind =
            tokenizer->nextToken.kind == TOKEN_OPERATOR_POS ? NODE_ADD : NODE_SUB;
        AST* result = newNode(resultKind, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseTerm(tokenizer);
        curr = result;
    }
    return curr;
}

AST* parseTerm(Tokenizer* tokenizer) {
    AST* root = parseFactor(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Unexpected end of file");
        }
        if (tokenizer->nextToken.kind != TOKEN_OPERATOR_MUL &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_DIV &&
                tokenizer->nextToken.kind != TOKEN_OPERATOR_MOD) {
            break;
        }
        NodeKind resultKind =
            tokenizer->nextToken.kind == TOKEN_OPERATOR_MUL ? NODE_MUL : 
            tokenizer->nextToken.kind == TOKEN_OPERATOR_DIV ? NODE_DIV : NODE_MOD;
        AST* result = newNode(resultKind, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        result->left = curr;
        result->right = parseFactor(tokenizer);
        curr = result;
    }
    return curr;
}

/* end of uninteresting functions */

AST* parseFactor(Tokenizer* tokenizer) {
    // if (!pollToken(tokenizer)) return NULL;
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                     "Unexpected end of file");
    }
    AST* lval;
    lval = parseCall(tokenizer);
    if (lval != NULL) return lval;
    lval = parseLvalue(tokenizer);
    if (lval != NULL) return lval;
    if (tokenizer->nextToken.kind == TOKEN_INTEGER_LITERAL ||
            tokenizer->nextToken.kind == TOKEN_FLOAT_LITERAL ||
            tokenizer->nextToken.kind == TOKEN_STRING_LITERAL ||
            tokenizer->nextToken.kind == TOKEN_CHAR_LITERAL) {
        NodeKind resultKind = 
            tokenizer->nextToken.kind == TOKEN_INTEGER_LITERAL ? NODE_INTEGER :
            tokenizer->nextToken.kind == TOKEN_FLOAT_LITERAL ? NODE_FLOAT :
            tokenizer->nextToken.kind == TOKEN_STRING_LITERAL ? NODE_STRING : NODE_CHAR;
        AST* result = newNode(resultKind, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        return result;
    }
    else if (tokenizer->nextToken.kind == TOKEN_LPAREN) {
        Token lparen = tokenizer->nextToken;
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* expr = parseExpression(tokenizer);
        if (expr == NULL) {
            compileError((FileInfo) { tokenizer->filename, lparen.pos },
                        "Invalid expression");
        }
        if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
            compileError((FileInfo) { tokenizer->filename, lparen.pos },
                        "Unmatched token");
        }
        tokenizer->nextToken.kind = TOKEN_NONE;
        return expr;
    }
    else if (tokenizer->nextToken.kind == TOKEN_OPERATOR_NEG) {
        AST* neg = newNode(NODE_NEG, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* fact = parseFactor(tokenizer);
        neg->left = fact;
        return neg;
    }
    else if (tokenizer->nextToken.kind == TOKEN_OPERATOR_NOT) {
        AST* not = newNode(NODE_NOT, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* fact = parseFactor(tokenizer);
        not->left = fact;
        return not;
    }

    return NULL;
}



// FIXME: almost definitely incorrect
AST* parseCall(Tokenizer* original) {
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                    "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) return NULL;
    AST* call = newNode(NODE_CALL, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    // similar to block
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                    "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LPAREN) return NULL;
    tokenizer->nextToken.kind = TOKEN_NONE;

    call->left = newNode(NODE_ARGUMENT, (Token){0});
    AST* curr = call->left;


    AST* firstArg = parseExpression(tokenizer);
    if (firstArg == NULL) {
        assert(pollToken(tokenizer)); // redundant
        if (tokenizer->nextToken.kind == TOKEN_RPAREN) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            *original = current;
            return call;
        }
        else {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Invalid argument");
        }
    }
    curr->right = newNode(NODE_ARGUMENT, (Token){0});
    curr->left = firstArg;
    curr = curr->right;

    while (1) {
        if (!pollToken(tokenizer)) {
            compileError((FileInfo) { tokenizer->filename, call->token.pos },
                        "Unmatched token");
        }
        if (tokenizer->nextToken.kind == TOKEN_RPAREN) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            break;
        }
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_COMMA) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            curr->right = newNode(NODE_ARGUMENT, (Token){0});
            curr->left = parseExpression(tokenizer);
            if (curr->left == NULL) {
                compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                            "Invalid argument");
            }
            curr = curr->right;
        }
        else {
            compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                        "Expected comma separator");
        }
    }

    *original = current;
    return call;
}




// null if no type, otherwise type lvalue
// if array, left will be enclosed expr
AST* parseLvalue(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                    "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) return NULL;

    // it is an lvalue
    AST* lvalue = newNode(NODE_LVALUE, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;

    // check if it is an array
    AST* expr = parseSubscript(tokenizer);
    if (expr != NULL) {
        lvalue->left = expr;
    }

    return lvalue;
}

// equivalent to above function
AST* parseType(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) {
        return NULL;
        // compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
        //             "Unexpected end of file (PARSE TYPE)");
    }
    if (tokenizer->nextToken.kind != TOKEN_TYPE) return NULL;

    // it is a type
    AST* type = newNode(NODE_TYPE, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;

    // check if it is an array
    AST* expr = parseSubscript(tokenizer);
    if (expr != NULL) {
        type->left = expr;
    }

    return type;
}


// returns the expression node
// caller should use it appropriately (lvalue, array decl)
AST* parseSubscript(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                    "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LSQUARE) return NULL;

    // it is an index
    Token lsquare = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    AST* expr = parseExpression(tokenizer);
    if (expr == NULL) {
        compileError((FileInfo) { tokenizer->filename, lsquare.pos },
                    "Expected expression after [");
    }

    if (!pollToken(tokenizer)) {
        compileError((FileInfo) { tokenizer->filename, tokenizer->nextToken.pos },
                    "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_RSQUARE) {
        compileError((FileInfo) { tokenizer->filename, lsquare.pos },
                    "Unmatched token");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;

    return expr;
}

