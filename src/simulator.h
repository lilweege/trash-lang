#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "parser.h"
#include "hashmap.h"

#define STACK_SIZE (256 * 1000)

// ideally there would be some bytecode to interpret
// for now, altough inefficient, simply reuse the AST
void  simulateProgram     (const char* filename, AST* program, size_t stackSize);
void  simulateStatements  (const char* filename, AST* statement,  HashMap* symbolTable);
void  simulateConditional (const char* filename, AST* statement,  HashMap* symbolTable);
void  simulateStatement   (const char* filename, AST* wrapper,    HashMap* symbolTable);
Value evaluateCall        (const char* filename, AST* call,       HashMap* symbolTable);
Value evaluateExpression  (const char* filename, AST* expression, HashMap* symbolTable);

#endif // SIMULATOR_H
