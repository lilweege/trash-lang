#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "parser.h"
#include "hashmap.h"

#define STACK_SIZE (256 * 1000)

// ideally there would be some bytecode to interpret
// for now, altough inefficient, simply reuse the AST
void simulateProgram(AST* program, size_t maxStackSize);
void simulateStatements(AST* statement, HashMap* symbolTable);
void simulateConditional(AST* statement, HashMap* symbolTable);
void simulateBlock(AST* body, HashMap* symbolTable);
void simulateStatement(AST* wrapper, HashMap* symbolTable);
Value evaluateCall(AST* call, HashMap* symbolTable);
Value evaluateExpression(AST* expression, HashMap* symbolTable);

#endif // SIMULATOR_H
