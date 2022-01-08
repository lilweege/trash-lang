#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

const char* nodeKindName(NodeKind kind) {
    static_assert(NODE_COUNT == 30, "Exhaustive check of node kinds failed");
    const char* NodeKindNames[30] = {
        "NODE_IF",
        "NODE_ELSE",
        "NODE_WHILE",
        "NODE_BLOCK",
        "NODE_STATEMENT",
        "NODE_CALL",
        "NODE_DEFINITION",
        "NODE_ARGUMENT",
        "NODE_IDENTIFIER",
        "NODE_TYPE",
        "NODE_NUMBER",
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


#define SCRATCH_SIZE 100
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

#define failAtToken(tokenizer, token, msg) do { \
    tokenizerFailAt(*(tokenizer), (token).lineNo, (token).colNo, \
            msg" <%s:%d: %s:%zu:%zu: \""SV_FMT"\">", \
            __FILE__, __LINE__, \
            tokenKindName((token).kind), \
            (token).lineNo+1, \
            (token).colNo+1, \
            SV_ARG((token).text)); \
} while (0)

void printAST(AST* root, size_t depth) {
    if (root == NULL) return;
    for (size_t i = 0; i < depth; ++i) {
        putchar('\t');
    }
    printf("[%s]", nodeKindName(root->kind));
    if (root->kind == NODE_NUMBER || root->kind == NODE_IDENTIFIER || root->kind == NODE_STRING || root->kind == NODE_CHAR || root->kind == NODE_DEFINITION || root->kind == NODE_TYPE || root->kind == NODE_STATEMENT || root->kind == NODE_LVALUE /* || ...*/) {
        printf(": <%s: \""SV_FMT"\">", tokenKindName(root->token.kind), SV_ARG(root->token.text));
    }
    printf("\n");
    printAST(root->left, depth + 1);
    printAST(root->right, depth + 1);
}

// TODO: fix leaks

AST* parseProgram(Tokenizer* tokenizer) {
    pollToken(tokenizer);
    AST* resultTree;
    do {
        resultTree = parseStatement(tokenizer);
        printf("next token = <%s: \""SV_FMT"\">\n",
            tokenKindName(tokenizer->nextToken.kind),
            SV_ARG(tokenizer->nextToken.text));
        printAST(resultTree, 0);
    } while (resultTree != NULL);
    return resultTree;
}

#define expectSemicolon(tokenizer, token) do { \
    if (!pollToken(tokenizer) || tokenizer->nextToken.kind != TOKEN_SEMICOLON) { \
        failAtToken(tokenizer, token, "Expected semicolon after token"); \
    } \
    tokenizer->nextToken.kind = TOKEN_NONE; \
} while (0)

AST* parseStatement(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind == TOKEN_SEMICOLON) {
        Token semi = tokenizer->nextToken;
        tokenizer->nextToken.kind = TOKEN_NONE;
        return newNode(NODE_STATEMENT, semi);
    }
    AST* result = parseBranch(tokenizer);
    if (result == NULL) {
        result = parseDefinition(tokenizer);
    }
    if (result == NULL) {
        result = parseAssignment(tokenizer);
    }
    if (result == NULL) {
        result = parseExpression(tokenizer);
        if (result == NULL) {
            // TODO: good error message
            // failAtToken(tokenizer, tokenizer->nextToken, "I don't know what this is, but it's not correct");
            return NULL;
        }
        expectSemicolon(tokenizer, result->token);
    }
    if (result == NULL) return NULL;
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
        tokenizer->nextToken.kind == TOKEN_ELSE ? NODE_ELSE : NODE_WHILE;
    AST* branch = newNode(branchKind, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LPAREN) {
        failAtToken(tokenizer, branch->token, "Expected ( after token");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    branch->left = parseExpression(tokenizer);
    if (branch->left == NULL) {
        failAtToken(tokenizer, branch->token, "Invalid expression after (");
    }
    
    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
        failAtToken(tokenizer, branch->token, "Expected ) after expression");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    branch->right = parseBlock(tokenizer);
    if (branch->right == NULL) {
        branch->right = parseStatement(tokenizer);
    }
    if (branch->right == NULL) {
        failAtToken(tokenizer, branch->token, "Expected conditional body");
    }
    return branch;
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
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }

    if (tokenizer->nextToken.kind != TOKEN_LCURLY) return NULL;
    AST* block = newNode(NODE_BLOCK, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    block->right = newNode(NODE_STATEMENT, (Token){0});
    AST* curr = block->right;

    while (1) {
        if (!pollToken(tokenizer)) {
            failAtToken(tokenizer, block->token, "Unmatched token");
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
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) {
        failAtToken(tokenizer, tokenizer->nextToken, "Invalid definition, expected identifier");
    }
    AST* defn = newNode(NODE_DEFINITION, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    expectSemicolon(tokenizer, tokenizer->nextToken);
    defn->left = type;
    return defn;
}

AST* parseAssignment(Tokenizer* original) {
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;
    
    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    AST* lvalue = parseLvalue(tokenizer);
    if (lvalue == NULL) return NULL;

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_OPERATOR_ASSIGN) return NULL;
    Token eq = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    AST* expr = parseExpression(tokenizer);
    if (expr == NULL) {
        // TODO: better error message
        failAtToken(tokenizer, eq, "Invalid assignment");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
            failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    AST* lval;
    lval = parseCall(tokenizer);
    if (lval != NULL) return lval;
    lval = parseLvalue(tokenizer);
    if (lval != NULL) return lval;
    if (tokenizer->nextToken.kind == TOKEN_INTEGER_LITERAL ||
            tokenizer->nextToken.kind == TOKEN_FLOAT_LITERAL) {
        AST* result = newNode(NODE_NUMBER, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        return result;
    }
    else if (tokenizer->nextToken.kind == TOKEN_STRING_LITERAL) {
        AST* result = newNode(NODE_STRING, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        return result;
    }
    else if (tokenizer->nextToken.kind == TOKEN_CHAR_LITERAL) {
        AST* result = newNode(NODE_CHAR, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        return result;
    }
    else if (tokenizer->nextToken.kind == TOKEN_LPAREN) {
        Token lparen = tokenizer->nextToken;
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* expr = parseExpression(tokenizer);
        if (expr == NULL) {
            failAtToken(tokenizer, lparen, "Invalid expression");
        }
        if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
            failAtToken(tokenizer, lparen, "Unmatched token");
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
    printf("\t\t\t\t\t\t\t\t\t\tPARSING CALL: START\n");
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) return NULL;
    tokenizer->nextToken.kind = TOKEN_NONE;
    printf("\t\t\t\t\t\t\t\t\t\tPARSING CALL: GOT IDENT\n");
    
    // similar to block
    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LPAREN) return NULL;
    Token lparen = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;
    printf("\t\t\t\t\t\t\t\t\t\tPARSING CALL: GOT LPAREN\n");

    AST* call = newNode(NODE_CALL, lparen);
    call->left = newNode(NODE_ARGUMENT, (Token){0});
    AST* curr = call->left;


    AST* firstArg = parseExpression(tokenizer);
    if (firstArg == NULL) {
        assert(pollToken(tokenizer)); // redundant
        if (tokenizer->nextToken.kind == TOKEN_RPAREN) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            return call;
        }
        else {
            failAtToken(tokenizer, tokenizer->nextToken, "Invalid argument");
        }
    }
    curr->right = newNode(NODE_ARGUMENT, (Token){0});
    curr->left = firstArg;
    curr = curr->right;
    printf("\t\t\t\t\t\t\t\t\t\tPARSING CALL: FIRST ARG\n");

    while (1) {
        if (!pollToken(tokenizer)) {
            failAtToken(tokenizer, call->token, "Unmatched token");
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
                failAtToken(tokenizer, tokenizer->nextToken, "Invalid argument");
            }
            curr = curr->right;
        }
        else {
            failAtToken(tokenizer, tokenizer->nextToken, "Expected comma separator");
        }
    }

    *original = current;
    return call;
}




// null if no type, otherwise type lvalue
// if array, left will be enclosed expr
AST* parseLvalue(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
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
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_LSQUARE) return NULL;

    // it is an index
    Token lsquare = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    AST* expr = parseExpression(tokenizer);
    if (expr == NULL) {
        failAtToken(tokenizer, lsquare, "Expected expression after [");
    }

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_RSQUARE) {
        failAtToken(tokenizer, lsquare, "Unmatched token");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;

    return expr;
}

