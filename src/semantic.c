#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "AST.h"
#include "semantic.h"
#include "debug.h"
#include "common.h"

#define NEST_DEPTH 20
SymbolList id_symlist, type_symlist;
Symbol cur_func = NULL;
int struct_env_dep = 0;

Symbol getIdDef(char *name) {
    Symbol cur_sym = id_symlist;
    while (cur_sym != NULL) {
        if (strcmp(cur_sym->name, name) == 0) return cur_sym;
        cur_sym = cur_sym->tail;
    }
    return NULL;
}

Symbol getTypeDef(char *name) {
    Symbol cur_sym = type_symlist;
    while (cur_sym != NULL) {
        if (strcmp(cur_sym->name, name) == 0) return cur_sym;
        cur_sym = cur_sym->tail;
    }
    return NULL;
}

int addIdDef(Symbol sym) {
    if (getIdDef(sym->name) != NULL) return -1;
    sym->tail = id_symlist;
    id_symlist = sym;
    if (sym->kind == FUNC_DEF) {
        cur_func = sym;
    }
    return 0;
}

int addTypeDef(Symbol sym) {
    if (getTypeDef(sym->name) != NULL) return -1;
    sym->tail = type_symlist;
    type_symlist = sym;
    return 0;
}

Type getType(ASTNode specifier) {
    Type type = NULL;
    if (specifier->subtype == TYPE_STRUCT) {
        if (specifier->child->subtype == NEW_STRUCT) {
            type = newType(specifier->child);
        }
        else {
            char *tag = specifier->child->child->sibling->child->val.c;
            Symbol sym = getTypeDef(tag);
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

Type newType(ASTNode structSpecifier) {
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
        if (addTypeDef(sym) < 0) {
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

void decExtVar(Type type, ASTNode extDecList) {
    ASTNode varDec = extDecList->child;
    Symbol sym = getSym4VarDec(type, varDec);
    if (addIdDef(sym) < 0) {
        reportError("3", extDecList->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (extDecList->subtype == UNFINISHED) {
        decExtVar(type, varDec->sibling->sibling);
    }
}

void decVar(Type type, ASTNode decList) {
    ASTNode dec = decList->child;
    Symbol sym = getSym4VarDec(type, dec->child);
    if (dec->subtype == INITIALIZE && typeEqual(sym->u.type, checkExpType(dec->child->sibling->sibling)) == 0) {
        reportError("5", dec->lineno, "Type mismatched for assignment");
    }
    if (addIdDef(sym) < 0) {
        reportError("3", dec->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (decList->subtype == UNFINISHED) {
        decVar(type, dec->sibling->sibling);
    }
}

void decFunc(Type type, ASTNode funDec) {
    Symbol sym = getSym4FuncDec(type, funDec);
    if (addIdDef(sym) < 0) {
        reportError("4", funDec->lineno, "Redefined function \"%s\"", sym->name);
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

Symbol getSym4FuncDec(Type type, ASTNode funDec) {
    Symbol sym = (Symbol)malloc(sizeof(struct SymbolList_));
    sym->kind = FUNC_DEF;
    strcpy(sym->name, funDec->child->val.c); 
    sym->u.func = (Func)malloc(sizeof(struct Func_));
    sym->u.func->retType = type;
    sym->u.func->argList = NULL;
    if (funDec->subtype != VOID_ARG) {
        sym->u.func->argList = buildArgs(NULL, funDec->child->sibling->sibling);
    }
    return sym;
}

FieldList buildArgs(FieldList argList, ASTNode varList) {
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
    if (addIdDef(sym) < 0) {
        reportError("3", varList->lineno, "Redefined variable \"%s\"", sym->name);
    }
    if (varList->subtype == UNFINISHED) {
        buildArgs(argList, paramDec->sibling->sibling);
    }
    return argList;
}

void semantic_parse(ASTNode parent) {
    if (parent == NULL) return;
    if (parent->type > AST_Program && parent->child == NULL) return;
    // before
    switch (parent->type) {
        case AST_ExtDef: {
            Type type = getType(parent->child);
            if (type == NULL) {
                reportError("17", parent->lineno, "Undefined structure \"%s\"", parent->child->child->child->sibling->child->val.c);
            }
            else if (parent->subtype == VAR_DEC) {
                decExtVar(type, parent->child->sibling);
            }
            else if (parent->subtype == FUNC_DEC) { // should be declared before check childs
                decFunc(type, parent->child->sibling);
            }
        }   break;
        case AST_StructSpecifier: {
            if (parent->subtype == NEW_STRUCT) {
                struct_env_dep ++;
            }
        }   break;
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
                decVar(type, parent->child->sibling);
            }
        }   break;
        case AST_StructSpecifier: {
            if (parent->subtype == NEW_STRUCT) {
                struct_env_dep --;
            }
        }   break;
        case AST_ID: {
            if (parent->subtype == VAR_USE) {
                if (getIdDef(parent->val.c) == NULL) {
                    reportError("1", parent->lineno, "Undefined variable \"%s\"", parent->val.c);
                }
            }
            else if(parent->subtype == FUNC_USE) {
                Symbol func_sym = getIdDef(parent->val.c);
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
                        if (isLeftVal(first) == 0) {
                            reportError("6", exp->lineno, "The left-hand side of an assignment must be a variable");
                        }
                        if (typeEqual(type, checkExpType(first->sibling->sibling)) == 0) {
                            reportError("5", exp->lineno, "Type mismatched for assignment");
                            type = NULL;
                        }
                    }   break;
                    case AST_AND:   case AST_OR:    case AST_RELOP: {
                        type = (Type)malloc(sizeof(struct Type_));
                        type->kind = BASIC;
                        type->u.basic = TYPE_INT;
                    }   break;
                    case AST_PLUS:  case AST_MINUS: case AST_STAR:  case AST_DIV: {
                        if (typeEqual(type, checkExpType(first->sibling->sibling)) == 0) {
                            reportError("7", exp->lineno, "Type mismatched for operands");
                            type = NULL;
                        }
                    }   break;
                    default:    panic("Unknown Exp type");
                }
            }
        }   break;
        case AST_MINUS: case AST_LP:    type = checkExpType(first->sibling);   break;
        case AST_ID: {
            Symbol sym = getIdDef(first->val.c);
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
        case AST_INT: case AST_NOT: {
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

int isLeftVal(ASTNode exp) {
    ASTNode first = exp->child;
    switch (first->type) {
        case AST_Exp: {
            if (first->sibling->type == AST_LB || first->sibling->type == AST_DOT
            || first->sibling->type == AST_ASSIGNOP) {
                return 1;
            }
            return 0;
        }   break;
        case AST_LP:    return isLeftVal(first->sibling);   break;
        case AST_ID: {
            if (first->subtype == FUNC_USE) return 0;
            return 1;
        }   break;
        default:        return 0;
    }
    return 0;
}

int typeEqual(Type t1, Type t2) {
    if (t1 == NULL || t2 == NULL) return 1;
    if (t1->kind != t2->kind) return 0;
    if (t1->kind == BASIC) return t1->u.basic == t2->u.basic;
    if (t1->kind == STRUCTURE) return structEqual(t1->u.structure, t2->u.structure);
    // if (t1->kind == ARRAY) return typeEqual(t1->u.array.elem, t2->u.array.elem);
    panic("Unknown Type");
}

int structEqual(FieldList st1, FieldList st2) {
    if (st1 == NULL && st2 == NULL) return 1;
    if (st1 == NULL || st2 == NULL) return 0;
    if (typeEqual(st1->type, st2->type) == 0) return 0;
    return structEqual(st1->tail, st2->tail);
}

void checkStmtType(ASTNode stmt) {
    if (stmt->child->type == AST_RETURN) {
        assert(cur_func);
        Type retType = cur_func->u.func->retType;
        if (typeEqual(retType, checkExpType(stmt->child->sibling)) == 0) {
            reportError("8", stmt->lineno, "Type mismatched for return");
        }
    }
}

void checkArgs(FieldList argList, ASTNode args) {
    int matched = 1;
    assert(args);   // args->child == NULL if args is void
    if (args->child == NULL) {
        if (argList != NULL) matched = 0;
    }
    else if (argList == NULL) matched = 0;
    else {
        matched = typeEqual(argList->type, checkExpType(args->child));
        if (matched) {
            if (args->subtype == UNFINISHED) {
                checkArgs(argList->tail, args->child->sibling->sibling);
                return;
            }
            else if (argList->tail != NULL) matched = 0;
        }
    }
    if (matched == 0) {
        reportError("9", args->lineno, "Function call is not applicable");
    }
}