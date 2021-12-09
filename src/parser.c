#include "parser.h"

#include <stdlib.h>
#include <stdio.h>


AST scratchBuffer[100];
AST* newNode(NodeKind kind, Token token) {
    // TODO: perhaps store these somewhere local
    // AST* node = (AST*) malloc(sizeof(AST)); // assume this malloc never fails for now
    static int sz = 0;
    AST* node = scratchBuffer + sz++;
    node->kind = kind;
    node->token = token;
    node->left = node->right = NULL;
    return node;
}

void parseTokens(Tokenizer tokenizer) {
    AST* resultTree = parseExpression(&tokenizer);
    printf("DONE: &resultTree = %p\n", (void*)resultTree);
    if (!pollToken(&tokenizer)) {
        return;
    }
    if (tokenizer.nextToken.kind != TOKEN_NEWLINE) {
        tokenizerFail(tokenizer, "Unexpected token at end of expression");
    }
    printf("next token = <%s:\""SV_FMT"\">\n",
            TokenKindNames[tokenizer.nextToken.kind],
            SV_ARG(tokenizer.nextToken.text));
    printAST(resultTree, 1);
}

void printAST(AST* root, size_t depth) {
    if (root == NULL) return;
    for (size_t i = 0; i < depth; ++i) {
        putchar('\t');
    }
    printf("[%s]", NodeKindNames[root->kind]);
    if (root->kind == NODE_NUMBER || root->kind == NODE_IDENTIFIER /* || ...*/) {
        printf(": <%s:"SV_FMT">", TokenKindNames[root->token.kind], SV_ARG(root->token.text));
    }
    printf("\n");
    printAST(root->left, depth + 1);
    printAST(root->right, depth + 1);
}

AST* parseExpression(Tokenizer* tokenizer) {
    AST* root = parseTerm(tokenizer);
    if (root == NULL) { return NULL; }
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) { return curr; }
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_POS) {
            AST* result = newNode(NODE_ADD, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseTerm(tokenizer);
            curr = result;
        }
        else if (tokenizer->nextToken.kind == TOKEN_OPERATOR_NEG) {
            AST* result = newNode(NODE_SUB, tokenizer->nextToken);
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
    if (root == NULL) { return NULL; }
    AST* curr = root;
    while (1) {
        if (!pollToken(tokenizer)) { return curr; }
        if (tokenizer->nextToken.kind == TOKEN_OPERATOR_MUL) {
            AST* result = newNode(NODE_MUL, tokenizer->nextToken);
            tokenizer->nextToken.kind = TOKEN_NONE;
            result->left = curr;
            result->right = parseFactor(tokenizer);
            curr = result;
        }
        else if (tokenizer->nextToken.kind == TOKEN_OPERATOR_DIV) {
            AST* result = newNode(NODE_DIV, tokenizer->nextToken);
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
    if (!pollToken(tokenizer)) { return NULL; }
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
        if (!pollToken(tokenizer)) { return NULL; }
        tokenizer->nextToken.kind = TOKEN_NONE;
        AST* expr = parseExpression(tokenizer);
        if (expr == NULL) {
            return NULL;
        }
        // idk if this is correct ??? maybe poll another token
        if (tokenizer->nextToken.kind != TOKEN_RPAREN) {
            return NULL;
        }
        if (!pollToken(tokenizer)) { return NULL; }
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

    return NULL;
}
