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

void parseTokens(Tokenizer tokenizer) {
    AST* resultTree;
    do {
        resultTree = parseStatement(&tokenizer);
        printAST(resultTree, 0);
    } while (resultTree != NULL);
}

void printAST(AST* root, size_t depth) {
    if (root == NULL) return;
    for (size_t i = 0; i < depth; ++i) {
        putchar('\t');
    }
    printf("[%s]", NodeKindNames[root->kind]);
    if (root->kind == NODE_NUMBER || root->kind == NODE_IDENTIFIER /* || ...*/) {
        printf(": <%s: \""SV_FMT"\">", TokenKindNames[root->token.kind], SV_ARG(root->token.text));
    }
    printf("\n");
    printAST(root->left, depth + 1);
    printAST(root->right, depth + 1);
}

// TODO: fix leaks

AST* parseStatement(Tokenizer* tokenizer) {
    AST* result = parseAssignment(tokenizer);
    if (result == NULL) {
        result = parseExpression(tokenizer);
    }
    if (result == NULL) return NULL;
    if (tokenizer->nextToken.kind != TOKEN_NEWLINE && 
            tokenizer->nextToken.kind != TOKEN_NONE) {
        tokenizerFail(*tokenizer,
                "Unexpected token <%s: \""SV_FMT"\">",
                TokenKindNames[tokenizer->nextToken.kind],
                SV_ARG(tokenizer->nextToken.text));

    }
    tokenizer->nextToken.kind = TOKEN_NONE;
    return result;
}

AST* parseAssignment(Tokenizer* original) {
    Tokenizer current = *original;
    Tokenizer* tokenizer = &current;
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind != TOKEN_IDENTIFIER) return NULL;
    AST* lval = newNode(NODE_IDENTIFIER, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    if (!pollToken(tokenizer)) return NULL;
    if (tokenizer->nextToken.kind != TOKEN_OPERATOR_ASSIGN) return NULL;
    AST* result = newNode(NODE_ASSIGN, tokenizer->nextToken);
    tokenizer->nextToken.kind = TOKEN_NONE;
    AST* expr = parseExpression(tokenizer);
    if (expr == NULL) return NULL;
    result->left = lval;
    result->right = expr;
    *original = current;
    return result;
}

AST* parseExpression(Tokenizer* tokenizer) {
    AST* root = parseLogicalTerm(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_OR) {
            AST* result = newNode(NODE_OR, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseLogicalTerm(tokenizer);
            curr = result;
        }
        else {
            break;
        }
    }
    return curr;
}

AST* parseLogicalTerm(Tokenizer* tokenizer) {
    AST* root = parseLogicalFactor(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_AND) {
            AST* result = newNode(NODE_AND, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseLogicalFactor(tokenizer);
            curr = result;
        }
        else {
            break;
        }
    }
    return curr;
}

AST* parseLogicalFactor(Tokenizer* tokenizer) {
    AST* root = parseComparison(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_EQ ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_NE) {
            NodeKind resultKind =
                tokenizer->nextToken.kind == TOKEN_OPERATOR_EQ ? NODE_EQ : NODE_NE;
            AST* result = newNode(resultKind, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseComparison(tokenizer);
            curr = result;
        }
        else {
            break;
        }
    }
    return curr;
}

AST* parseComparison(Tokenizer* tokenizer) {
    AST* root = parseValue(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_GE ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_LE ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_GT ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_LT) {
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
        else {
            break;
        }
    }
    return curr;
}

AST* parseValue(Tokenizer* tokenizer) {
    AST* root = parseTerm(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_POS ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_NEG) {
            NodeKind resultKind =
                tokenizer->nextToken.kind == TOKEN_OPERATOR_POS ? NODE_ADD : NODE_SUB;
            AST* result = newNode(resultKind, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseTerm(tokenizer);
            curr = result;
        }
        else {
            break;
        }
    }
    return curr;
}

AST* parseTerm(Tokenizer* tokenizer) {
    AST* root = parseFactor(tokenizer);
    if (root == NULL) return NULL;
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) return curr;
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_MUL ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_DIV ||
                tokenizer->nextToken.kind == TOKEN_OPERATOR_MOD) {
            NodeKind resultKind =
                tokenizer->nextToken.kind == TOKEN_OPERATOR_MUL ? NODE_MUL : 
                tokenizer->nextToken.kind == TOKEN_OPERATOR_DIV ? NODE_DIV : NODE_MOD;
            AST* result = newNode(resultKind, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseFactor(tokenizer);
            curr = result;
        }
        else {
            break;
        }
    }
    return curr;
}

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
        if (!pollToken(tokenizer)) return NULL;
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* expr = parseExpression(tokenizer);
        if (expr == NULL) {
            return NULL;
        }
        // the next token should already be an rparen
        if (!pollToken(tokenizer)) return NULL; // this line is probably redundant
        if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
            return NULL;
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
