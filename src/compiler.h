#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void compileArguments(int argc, char** argv);
void compileFile(char* filename);
void simulateFile(char* filename);

#endif // COMPILER_H
