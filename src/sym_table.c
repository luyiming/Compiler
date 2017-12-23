#include <string.h>
#include <assert.h>
#include "sym_table.h"
#include "rb_tree.h"
#include "semantic.h"
#include "AST.h"
#include "common.h"

struct rb_tree *symbolTable[MAX_NEST_DEPTH];
int currentNestedDepth = 0;

int symbol_cmp(struct rb_tree *self, struct rb_node *node_a,
               struct rb_node *node_b) {
    Symbol a = (Symbol)node_a->value;
    Symbol b = (Symbol)node_b->value;
    return strcmp(a->name, b->name);
}

void initSymbolTabel() {
    symbolTable[0] = rb_tree_create(symbol_cmp);
}

void enterScope() {
#ifdef ENABLE_NESTED_SCOPE
    currentNestedDepth++;
    symbolTable[currentNestedDepth] = rb_tree_create(symbol_cmp);
#endif
}

void leaveScope() {
#ifdef ENABLE_NESTED_SCOPE
    rb_tree_dealloc(symbolTable[currentNestedDepth], NULL);
    currentNestedDepth--;
#endif
}

Symbol lookupSymbol(char *name, bool checkUpperScope) {
    Symbol tmp = &(struct SymbolList_){.name = {'\n'}};
    strcpy(tmp->name, name);

    if (checkUpperScope == false)
        return rb_tree_find(symbolTable[currentNestedDepth], tmp);

    for (int depth = currentNestedDepth; depth >= 0; depth--) {
        Symbol sym = rb_tree_find(symbolTable[depth], tmp);
        if (sym) return sym;
    }
    return NULL;
}

Symbol lookupType(char *name, bool checkUpperScope) {
    Symbol tmp = &(struct SymbolList_){.name = {'\n'}};
    strcpy(tmp->name, name);

    if (checkUpperScope == false)
        return rb_tree_find(symbolTable[currentNestedDepth], tmp);

    for (int depth = currentNestedDepth; depth >= 0; depth--) {
        Symbol sym = rb_tree_find(symbolTable[depth], tmp);
        if (sym) return sym;
    }
    return NULL;
}

int insertSymbol(Symbol sym) {
    Symbol oldsym;
    if (sym->kind == FUNC_DEF) {
#ifdef ENABLE_NESTED_SCOPE
        assert(currentNestedDepth == 1);
        // TODO: remove this hack
        // 函数符号应该在最外层作用域，但是我们在进入 FunDec 的时候已经进入了内层作用域，
        // 此时添加函数符号仍然应该添加到外层，并且查找函数的时候也应该从最外层查找
        // 参见 20-4.txt 测试用例
        currentNestedDepth = 0;
        oldsym = lookupSymbol(sym->name, false);
        currentNestedDepth = 1;
#else
        oldsym = lookupSymbol(sym->name, false);
#endif
    } else {
        oldsym = lookupSymbol(sym->name, false);
    }
    if (oldsym != NULL) {
        if (oldsym->kind == FUNC_DEF && sym->kind == FUNC_DEF) {
            if (oldsym->u.func->definition != NULL && sym->u.func->definition != NULL) {
                // 函数重定义
                return -1;
            }
            if (funcSignitureEqual(oldsym, sym) == false) {
                // 函数声明/定义签名不一致
                return -2;
            }
            if (sym->u.func->definition)
                oldsym->u.func->definition = sym->u.func->definition;
            return 0;
        }
        else {
            return -1;
        }
    }
    if (sym->kind == FUNC_DEF) {
        rb_tree_insert(symbolTable[0], sym);
    } else {
        rb_tree_insert(symbolTable[currentNestedDepth], sym);
    }
    return 0;
}

int insertType(Symbol sym) {
    if (lookupType(sym->name, false) != NULL) return -1;
    rb_tree_insert(symbolTable[currentNestedDepth], sym);
    return 0;
}
