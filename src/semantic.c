#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "AST.h"
#include "semantic.h"
#include "debug.h"
#include "common.h"
#include "rb_tree.h"

#define MAX_NEST_DEPTH 20
struct rb_tree *symbolTable[MAX_NEST_DEPTH];
int currentNestedDepth = 0;
Symbol cur_func = NULL;
int struct_env_dep = 0;

int symbol_cmp(struct rb_tree *self, struct rb_node *node_a,
               struct rb_node *node_b) {
    Symbol a = (Symbol)node_a->value;
    Symbol b = (Symbol)node_b->value;
    return strcmp(a->name, b->name);
}

void initSymbolTabel() {
    symbolTable[0] = rb_tree_create(symbol_cmp);
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
        cur_func = sym;
        assert(currentNestedDepth == 1);
        // TODO: remove this hack
        // 函数符号应该在最外层作用域，但是我们在进入 FunDec 的时候已经进入了内层作用域，
        // 此时添加函数符号仍然应该添加到外层，并且查找函数的时候也应该从最外层查找
        // 参见 20-4.txt 测试用例
        currentNestedDepth = 0;
        oldsym = lookupSymbol(sym->name, false);
        currentNestedDepth = 1;
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

Type getType(ASTNode specifier) {
    Type type = NULL;
    if (specifier->subtype == TYPE_STRUCT) {
        if (specifier->child->subtype == NEW_STRUCT) {
            type = buildStructType(specifier->child);
        }
        else {
            char *tag = specifier->child->child->sibling->child->val.c;
            Symbol sym = lookupType(tag, true);
            if (sym != NULL) {
                type = sym->u.type;
            } // else: report 'undefined' somewhere else
        }
    }
    else {
        type = (Type)malloc(sizeof(struct Type_));
        type->kind = BASIC;
        char *typename = specifier->child->val.c;
        if (strcmp(typename, "int") == 0) type->u.basic = TYPE_INT;
        else if (strcmp(typename, "float") == 0) type->u.basic = TYPE_FLOAT;
        else {
            panic("Unknown basic type");
        }
    }
    return type;
}

Type buildStructType(ASTNode structSpecifier) {
    Type type = (Type)malloc(sizeof(struct Type_));
    type->kind = STRUCTURE;
    type->u.structure = NULL;
    ASTNode optTag = structSpecifier->child->sibling;
    ASTNode defList = optTag->sibling->sibling;
    type->u.structure = buildFields(type->u.structure, defList);
    if (optTag->subtype != EMPTY) {
        Symbol sym = (Symbol)malloc(sizeof(struct SymbolList_));
        sym->kind = STRUCT_DEF;
        strcpy(sym->name, optTag->child->val.c);
        sym->u.type = type;
        if (insertType(sym) < 0) {
            reportError("16", structSpecifier->lineno, "Duplicated name \"%s\"", sym->name);
        }
    }
    return type;
}

FieldList buildFields(FieldList structure, ASTNode defList) {
    if (defList->subtype == EMPTY) return structure;
    ASTNode def = defList->child;
    Type type = getType(def->child);
    ASTNode decList = def->child->sibling;
    while (1) {
        ASTNode dec = decList->child;
        Symbol sym = getSym4VarDec(type, dec->child);
        if (dec->subtype == INITIALIZE) {
            reportError("15", dec->lineno, "Attemp to initialize field \"%s\"", sym->name);
        }
        Field field = (Field)malloc(sizeof(struct FieldList_));
        strcpy(field->name, sym->name);
        field->type = sym->u.type;
        if (structure == NULL) {
            structure = field;
            structure->tail = NULL;
        }
        else {
            if (addField(structure, field) < 0) {
                reportError("15", dec->lineno, "Redefined field \"%s\"", field->name);
            }
        }
        if (decList->subtype == UNFINISHED) {
            decList = dec->sibling->sibling;
        }
        else break;
    }
    return buildFields(structure, def->sibling);
}

int addField(FieldList structure, Field field) {
    assert(structure != NULL);
    FieldList cur_field = structure;
    while (cur_field != NULL) {
        if (strcmp(cur_field->name, field->name) == 0) return -1;
        if (cur_field->tail == NULL) break; // find list_tail
        cur_field = cur_field->tail;
    }
    cur_field->tail = field;
    field->tail = NULL;
    return 0;
}

Field getField(FieldList structure, char *name) {
    Field cur_field = structure;
    while (cur_field != NULL) {
        if (strcmp(cur_field->name, name) == 0) return cur_field;
        cur_field = cur_field->tail;
    }
    return NULL;
}

void parseExtDecList(Type type, ASTNode extDecList) {
    ASTNode varDec = extDecList->child;
    Symbol sym = getSym4VarDec(type, varDec);
    if (insertSymbol(sym) < 0) {
        reportError("3", extDecList->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (extDecList->subtype == UNFINISHED) {
        parseExtDecList(type, varDec->sibling->sibling);
    }
}

void parseDecList(Type type, ASTNode decList) {
    ASTNode dec = decList->child;
    Symbol sym = getSym4VarDec(type, dec->child);
    if (dec->subtype == INITIALIZE && typeEqual(sym->u.type, checkExpType(dec->child->sibling->sibling)) == false) {
        reportError("5", dec->lineno, "Type mismatched for assignment");
    }
    if (insertSymbol(sym) < 0) {
        reportError("3", dec->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (decList->subtype == UNFINISHED) {
        parseDecList(type, dec->sibling->sibling);
    }
}

void parseFunDec(Type type, ASTNode funDec) {
    Symbol sym = getSym4FunDec(type, funDec);
    int ret = insertSymbol(sym);
    if (ret == -1) {
        reportError("4", funDec->lineno, "Redefined function \"%s\"", sym->name);
    }
    else if (ret == -2) {
        reportError("19", funDec->lineno, "Function \"%s\" signiture conflict", sym->name);
    }
}

Symbol getSym4VarDec(Type type, ASTNode varDec) {
    Symbol sym = (Symbol)malloc(sizeof(struct SymbolList_));
    if (varDec->subtype == TYPE_ARRAY) {
        Symbol subSym = getSym4VarDec(type, varDec->child);
        Type arr = (Type)malloc(sizeof(struct Type_));
        arr->kind = ARRAY;
        arr->u.array.elem = subSym->u.type;
        arr->u.array.size = varDec->child->sibling->sibling->val.i;
        sym->kind = VAR_DEF;
        strcpy(sym->name, subSym->name);
        sym->u.type = arr;
    }
    else {
        sym->kind = VAR_DEF;
        strcpy(sym->name, varDec->child->val.c);
        sym->u.type = type;
    }
    return sym;
}

Symbol getSym4FunDec(Type type, ASTNode funDec) {
    Symbol sym = (Symbol)malloc(sizeof(struct SymbolList_));
    sym->kind = FUNC_DEF;
    strcpy(sym->name, funDec->child->val.c);
    sym->u.func = (Func)malloc(sizeof(struct Func_));
    sym->u.func->retType = type;
    sym->u.func->argList = NULL;
    sym->u.func->lineno = funDec->lineno;
    bool addToSymbolTabel = true;
    if (funDec->sibling->type == AST_SEMI) { // 函数声明
        sym->u.func->definition = NULL;
        addToSymbolTabel = false;
    } else { // 函数定义
        sym->u.func->definition = funDec->sibling; // CompSt
    }
    if (funDec->subtype != VOID_ARG) {
        sym->u.func->argList = buildArgs(NULL, funDec->child->sibling->sibling, addToSymbolTabel);
    }
    return sym;
}

FieldList buildArgs(FieldList argList, ASTNode varList, bool addToSymbolTabel) {
    ASTNode paramDec = varList->child;
    Type type = getType(paramDec->child);
    Symbol sym = getSym4VarDec(type, paramDec->child->sibling);
    Field field = (Field)malloc(sizeof(struct FieldList_));
    strcpy(field->name, sym->name);
    field->type = sym->u.type;
    if (argList == NULL) {
        argList = field;
        argList->tail = NULL;
    }
    else if (addField(argList, field) < 0) {
        reportError("3", varList->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (addToSymbolTabel == true) {
        if (insertSymbol(sym) < 0) {
            reportError("3", varList->lineno, "Redefined variable \"%s\"", sym->name);
        }
    }
    if (varList->subtype == UNFINISHED) {
        buildArgs(argList, paramDec->sibling->sibling, addToSymbolTabel);
    }
    return argList;
}

void semantic_parse(ASTNode parent) {
    if (parent == NULL) return;
    if (parent->type == AST_Program) {
        initSymbolTabel();
    }
    if (parent->type > AST_Program && parent->child == NULL) return;
    // before
    switch (parent->type) {
        case AST_ExtDef: {
            Type type = getType(parent->child);
            if (type == NULL) {
                reportError("17", parent->lineno, "Undefined structure \"%s\"", parent->child->child->child->sibling->child->val.c);
            }
            else if (parent->subtype == VAR_DEC) {
                parseExtDecList(type, parent->child->sibling);
            }
            else if (parent->subtype == FUNC_DEC) { // should be declared before check childs
                enterScope();
                parseFunDec(type, parent->child->sibling);
            }
        }   break;
        case AST_StructSpecifier: {
            if (parent->subtype == NEW_STRUCT) {
                struct_env_dep ++;
            }
        }   break;
        case AST_CompSt: {
            if (parent->parent->type != AST_ExtDef) {
                // 如果是 AST_ExtDef，表示这个 CompSt 是一个函数的 CompSt，而函数的
                // 作用域在 FunDec 的时候就已经进入了
                enterScope();
            }
        }   break;
        default:
            break;
    }
    // go down
    for (ASTNode p = parent->child; p != NULL; p = p->sibling) {
        semantic_parse(p);
    }
    // after
    // /**/printf("sem_parse:%s\n", ASTNodeTypeName[parent->type]);
    switch (parent->type) {
        case AST_Def: {
            if (struct_env_dep > 0) break;
            Type type = getType(parent->child);
            if (type == NULL) {
                reportError("17", parent->lineno, "Undefined structure \"%s\"", parent->child->child->child->sibling->child->val.c);
            }
            else {
                parseDecList(type, parent->child->sibling);
            }
        }   break;
        case AST_StructSpecifier: {
            if (parent->subtype == NEW_STRUCT) {
                struct_env_dep --;
            }
        }   break;
        case AST_ID: {
            if (parent->subtype == VAR_USE) {
                if (lookupSymbol(parent->val.c, true) == NULL) {
                    reportError("1", parent->lineno, "Undefined variable \"%s\"", parent->val.c);
                }
            }
            else if(parent->subtype == FUNC_USE) {
                Symbol func_sym = lookupSymbol(parent->val.c, true);
                if (func_sym == NULL) {
                    reportError("2", parent->lineno, "Undefined function \"%s\"", parent->val.c);
                }
                else if (func_sym->kind != FUNC_DEF) {
                    reportError("11", parent->lineno, "\"%s\" is not a function", parent->val.c);
                }
                else {
                    checkArgs(func_sym->u.func->argList, parent->sibling->sibling);
                }
            }
        }   break;
        case AST_Exp:   checkExpType(parent); break;
        case AST_Stmt:  checkStmtType(parent);  break;
        case AST_FunDec: {
            if (parent->sibling->type == AST_SEMI) {
                // 如果是函数声明，则作用域在这个时候退出
                // 否则如果是函数定义，则作用域在 CompSt 结束的时候退出
                leaveScope();
            }
        }   break;
        case AST_CompSt: {
            leaveScope();
        }   break;
        case AST_Program: {
            checkUndefinedFunc();
        }   break;
        default:
            break;
    }
}

Type checkExpType(ASTNode exp) {
    if (exp->expType != UNCHECKED) return exp->expType;
    Type type = NULL;
    ASTNode first = exp->child;
    switch (first->type) {
        case AST_Exp: {
            type = checkExpType(first);
            if (first->subtype == ARRAY_USE) {
                type = checkExpType(first);
                if (type != NULL) {
                    if (type->kind != ARRAY) {
                        reportError("10", exp->lineno, "Variable is not an array");
                        type = NULL;
                    }
                    else {
                        Type idx_type = checkExpType(first->sibling->sibling);
                        if (idx_type->kind != BASIC || idx_type->u.basic != TYPE_INT) {
                            reportError("12", first->lineno, "Index is not an integer");
                        }
                        type = type->u.array.elem;
                    }
                }
            }
            else if (first->subtype == STRUCT_USE) {
                Type fst_type = checkExpType(first);
                if (fst_type != NULL) {
                    if (fst_type->kind != STRUCTURE) {
                        reportError("13", exp->lineno, "Illegal use of \".\"");
                        type = NULL;
                    }
                    else {
                        Field field = getField(fst_type->u.structure, first->sibling->sibling->val.c);
                        if (field == NULL) {
                            reportError("14", first->lineno, "Non-existent field \"%s\"", first->sibling->sibling->val.c);
                            type = NULL;
                        }
                        else {
                            type = field->type;
                        }
                    }
                }
            }
            else {
                switch (first->sibling->type) {
                    case AST_ASSIGNOP: {
                        if (isLeftVal(first) == false) {
                            reportError("6", exp->lineno, "The left-hand side of an assignment must be a variable");
                        }
                        if (typeEqual(type, checkExpType(first->sibling->sibling)) == false) {
                            reportError("5", exp->lineno, "Type mismatched for assignment");
                            type = NULL;
                        }
                    }   break;
                    case AST_AND:   case AST_OR: {
                        if ((type != NULL && (type->kind != BASIC || type->u.basic != TYPE_INT))
                        || typeEqual(type, checkExpType(first->sibling->sibling)) == false) {
                            reportError("7", exp->lineno, "Type mismatched for boolean operator");
                            type = NULL;
                        }
                    }   break;
                    case AST_RELOP: {
                        if ((type != NULL && type->kind != BASIC)
                        || typeEqual(type, checkExpType(first->sibling->sibling)) == false) {
                            reportError("7", exp->lineno, "Type mismatched for operands");
                            type = NULL;
                        }
                        else {
                            type = (Type)malloc(sizeof(struct Type_));
                            type->kind = BASIC;
                            type->u.basic = TYPE_INT;
                        }
                    }   break;
                    case AST_PLUS:  case AST_MINUS: case AST_STAR:  case AST_DIV: {
                        if ((type != NULL && type->kind != BASIC)
                        || typeEqual(type, checkExpType(first->sibling->sibling)) == false) {
                            reportError("7", exp->lineno, "Type mismatched for operands");
                            type = NULL;
                        }
                    }   break;
                    default:    panic("Unknown Exp type");
                }
            }
        }   break;
        case AST_LP:    type = checkExpType(first->sibling);   break;
        case AST_MINUS: {
            type = checkExpType(first->sibling);
            if (type != NULL && type->kind != BASIC) {
                reportError("7", exp->lineno, "Type mismatched for arithmetic operator");
                type = NULL;
            }
        }   break;
        case AST_NOT: {
            type = checkExpType(first->sibling);
            if (type != NULL && (type->kind != BASIC || type->u.basic != TYPE_INT)) {
                reportError("7", exp->lineno, "Type mismatched for boolean operator");
                type = NULL;
            }
        }   break;
        case AST_ID: {
            Symbol sym = lookupSymbol(first->val.c, true);
            if (sym == NULL) {
                // reportError somewhere else
                type = NULL;
            }
            else if (first->subtype == FUNC_USE) {
                type = sym->u.func->retType;
            }
            else {
                type = sym->u.type;
            }
        }   break;
        case AST_INT: {
            type = (Type)malloc(sizeof(struct Type_));
            type->kind = BASIC;
            type->u.basic = TYPE_INT;
        }   break;
        case AST_FLOAT: {
            type = (Type)malloc(sizeof(struct Type_));
            type->kind = BASIC;
            type->u.basic = TYPE_FLOAT;
        }   break;
        default:        panic("Unknown Exp type");
    }
    exp->expType = type;
    return type;
}

bool isLeftVal(ASTNode exp) {
    ASTNode first = exp->child;
    switch (first->type) {
        case AST_Exp: {
            if (first->sibling->type == AST_LB ||
                first->sibling->type == AST_DOT ||
                first->sibling->type == AST_ASSIGNOP) {
                return true;
            }
            return false;
        } break;
        case AST_LP:
            return isLeftVal(first->sibling);
            break;
        case AST_ID: {
            if (first->subtype == FUNC_USE) return false;
            return true;
        } break;
        default:
            return false;
    }
    return false;
}

bool typeEqual(Type t1, Type t2) {
    if (t1 == NULL || t2 == NULL) return true;
    if (t1->kind != t2->kind) return false;
    if (t1->kind == BASIC) return t1->u.basic == t2->u.basic;
    if (t1->kind == STRUCTURE) return structEqual(t1->u.structure, t2->u.structure);
    if (t1->kind == ARRAY) return typeEqual(t1->u.array.elem, t2->u.array.elem);
    panic("Unknown Type");
}

bool structEqual(FieldList st1, FieldList st2) {
    if (st1 == NULL && st2 == NULL) return true;
    if (st1 == NULL || st2 == NULL) return false;
    if (typeEqual(st1->type, st2->type) == false) return false;
    return structEqual(st1->tail, st2->tail);
}

void checkStmtType(ASTNode stmt) {
    ASTNode first = stmt->child;
    if (first->type == AST_RETURN) {
        assert(cur_func);
        Type retType = cur_func->u.func->retType;
        if (typeEqual(retType, checkExpType(stmt->child->sibling)) == false) {
            reportError("8", stmt->lineno, "Type mismatched for return");
        }
    }
    else if (first->type == AST_IF || first->type == AST_WHILE) {
        ASTNode exp = first->sibling->sibling;
        Type type = checkExpType(exp);
        if (type != NULL && (type->kind != BASIC || type->u.basic != TYPE_INT)) {
            reportError("7", exp->lineno, "Type mismatched for boolean expression");
        }
    }
}

void checkArgs(FieldList argList, ASTNode args) {
    bool matched = true;
    assert(args);   // args->child == NULL if args is void
    if (args->child == NULL) {
        if (argList != NULL) matched = false;
    }
    else if (argList == NULL) matched = false;
    else {
        matched = typeEqual(argList->type, checkExpType(args->child));
        if (matched) {
            if (args->subtype == UNFINISHED) {
                checkArgs(argList->tail, args->child->sibling->sibling);
                return;
            }
            else if (argList->tail != NULL) matched = false;
        }
    }
    if (matched == false) {
        reportError("9", args->lineno, "Function call is not applicable");
    }
}

bool funcSignitureEqual(Symbol func1, Symbol func2) {
    if (!typeEqual(func1->u.func->retType, func2->u.func->retType))
        return false;
    Field arg1 = func1->u.func->argList;
    Field arg2 = func2->u.func->argList;
    while (arg1 != NULL && arg2 != NULL) {
        if (!typeEqual(arg1->type, arg2->type))
            return false;
        arg1 = arg1->tail;
        arg2 = arg2->tail;
    }
    if (arg1 != NULL || arg2 != NULL) {
        return false;
    }
    return true;
}

void checkUndefinedFunc() {
    struct rb_iter *iter = rb_iter_create();
    if (iter) {
        assert(currentNestedDepth == 0);
        for (Symbol sym = rb_iter_first(iter, symbolTable[currentNestedDepth]);
             sym != NULL; sym = rb_iter_next(iter)) {
            if (sym->kind == FUNC_DEF && sym->u.func->definition == NULL) {
                reportError("18", sym->u.func->lineno, "Undefined Function");
            }
        }
        rb_iter_dealloc(iter);
    }
}

void enterScope() {
    currentNestedDepth++;
    symbolTable[currentNestedDepth] = rb_tree_create(symbol_cmp);
}

void leaveScope() {
    rb_tree_dealloc(symbolTable[currentNestedDepth], NULL);
    currentNestedDepth--;
}
