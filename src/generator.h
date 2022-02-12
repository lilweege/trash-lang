#ifndef GENERATOR_H
#define GENERATOR_H

#include "parser.h"
#include "hashmap.h"
#include "io.h"


typedef enum {
    TARGET_LINUX,
    TARGET_WINDOWS,
} Target;

void generateProgram(const char* outputFilename, AST* program, Target target);
void generateStatements(AST* statement, HashMap* symbolTable, FileWriter* asmWriter);
void generateConditional(AST* statement, HashMap* symbolTable, FileWriter* asmWriter);
void generateStatement(AST* wrapper, HashMap* symbolTable, FileWriter* asmWriter);
Value generateCall(AST* call, HashMap* symbolTable, FileWriter* asmWriter);
Value generateExpression(AST* expression, HashMap* symbolTable, FileWriter* asmWriter);

#endif // GENERATOR_H
