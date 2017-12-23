#ifndef __SYM_TABLE_H__
#define __SYM_TABLE_H__

#include "rb_tree.h"
#include "semantic.h"

typedef struct rb_tree SymbolTabel;

extern int currentNestedDepth;

#define MAX_NEST_DEPTH 20
extern struct rb_tree *symbolTable[MAX_NEST_DEPTH];

void initSymbolTabel();
void enterScope();
void leaveScope();
Symbol lookupSymbol(char *name, bool checkUpperScope);
Symbol lookupType(char *name, bool checkUpperScope);
int insertSymbol(Symbol sym);
int insertType(Symbol sym);

// #define ENABLE_NESTED_SCOPE

#endif // __SYM_TABLE_H__
