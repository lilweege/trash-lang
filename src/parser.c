#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
            TokenKindNames[(token).kind], \
            (token).lineNo+1, \
            (token).colNo+1, \
            SV_ARG((token).text)); \
} while (0)

void printAST(AST* root, size_t depth) {
    if (root == NULL) return;
    for (size_t i = 0; i < depth; ++i) {
        putchar('\t');
    }
    printf("[%s]", NodeKindNames[root->kind]);
    if (root->kind == NODE_NUMBER || root->kind == NODE_IDENTIFIER || root->kind == NODE_STRING /* || ...*/) {
        printf(": <%s: \""SV_FMT"\">", TokenKindNames[root->token.kind], SV_ARG(root->token.text));
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
            TokenKindNames[tokenizer->nextToken.kind],
            SV_ARG(tokenizer->nextToken.text));
        printAST(resultTree, 0);
    } while (resultTree != NULL);
    return resultTree;
}


AST* parseStatement(Tokenizer* tokenizer) {
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind == TOKEN_SEMICOLON) {
        Token semi = tokenizer->nextToken;
        tokenizer->nextToken.kind = TOKEN_NONE;
        return newNode(NODE_STATEMENT, semi);
    }
    AST* result = parseBranch(tokenizer);
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

        if (!pollToken(tokenizer) || tokenizer->nextToken.kind != TOKEN_SEMICOLON) {
            failAtToken(tokenizer, result->token, "Expected semicolon after token");
        }
        tokenizer->nextToken.kind = TOKEN_NONE;
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
    //  / |
    // A S0
    //  / |
    // B S1
    //  / |
    // C S2
    // ...

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    AST* block = newNode(NODE_BLOCK, tokenizer->nextToken);
    block->left = newNode(NODE_STATEMENT, tokenizer->nextToken);
    AST* curr = block->left;
    
    if (tokenizer->nextToken.kind != TOKEN_LCURLY) return NULL;
    tokenizer->nextToken.kind = TOKEN_NONE;

    while (1) {
        if (!pollToken(tokenizer)) {
            failAtToken(tokenizer, block->token, "Unmatched token");
        }
        if (tokenizer->nextToken.kind == TOKEN_RCURLY) {
            tokenizer->nextToken.kind = TOKEN_NONE;
            break;
        }
        curr->right = newNode(NODE_STATEMENT, tokenizer->nextToken);
        curr->left = parseStatement(tokenizer);
        curr = curr->right;
    }
    return block;
}

AST* parseAssignment(Tokenizer* original) {
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;
    
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) return NULL;
    Token id = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    if (!pollToken(tokenizer)) {
        failAtToken(tokenizer, tokenizer->nextToken, "Unexpected end of file");
    }
    if (tokenizer->nextToken.kind != TOKEN_OPERATOR_ASSIGN) return NULL;
    Token eq = tokenizer->nextToken;
    tokenizer->nextToken.kind = TOKEN_NONE;

    AST* rval = parseRvalue(tokenizer);
    if (rval == NULL || !pollToken(tokenizer) || tokenizer->nextToken.kind != TOKEN_SEMICOLON) {
        // TODO: better error message
        failAtToken(tokenizer, eq, "Invalid assignment");
    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    
    AST* lval = newNode(NODE_IDENTIFIER, id);
    AST* result = newNode(NODE_ASSIGN, eq);

    result->left = lval;
    result->right = rval;
    *original = current;
    return result;
}

AST* parseRvalue(Tokenizer* tokenizer) {
    // TODO: array
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind == TOKEN_STRING_LITERAL) {
        Token str = tokenizer->nextToken;
        tokenizer->nextToken.kind = TOKEN_NONE;
        return newNode(NODE_STRING, str);
    }
    return parseExpression(tokenizer);
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
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind == TOKEN_IDENTIFIER) {
        AST* result = newNode(NODE_IDENTIFIER, tokenizer->nextToken);
        tokenizer->nextToken.kind = TOKEN_NONE;
        return result;
    }
    else if (tokenizer->nextToken.kind == TOKEN_INTEGER_LITERAL ||
            tokenizer->nextToken.kind == TOKEN_DOUBLE_LITERAL) {
        AST* result = newNode(NODE_NUMBER, tokenizer->nextToken);
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
