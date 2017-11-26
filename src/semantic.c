#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "AST.h"
#include "semantic.h"
#include "debug.h"

#define NEST_DEPTH 20
SymbolList id_symlist, type_symlist;
Symbol cur_func = NULL;

int struct_env_dep = 0;

Symbol getIdDef(char *name) {
    Symbol cur_sym = id_symlist;
    while (cur_sym != NULL) {
        if (strcmp(cur_sym->name, name) == 0) {
            return cur_sym;
        }
        cur_sym = cur_sym->tail;
    }
    return NULL;
}

Symbol getTypeDef(char *name) {
    Symbol cur_sym = type_symlist;
    while (cur_sym != NULL) {
        if (strcmp(cur_sym->name, name) == 0) {
            return cur_sym;
        }
        cur_sym = cur_sym->tail;
    }
    return NULL;
}

int addIdDef(Symbol sym) {
    if (getIdDef(sym->name) != NULL) {
        return -1; // redefined
    }
    sym->tail = id_symlist;
    id_symlist = sym;
    if (sym->kind == FUNC_DEF) {
        cur_func = sym;
    }
    return 0;
}

int addTypeDef(Symbol sym) {
    SymbolList cur_sym = type_symlist;
    while (cur_sym != NULL) {
        if (strcmp(cur_sym->name, sym->name) == 0) {
            return -1;   // redefined
        }
        cur_sym = cur_sym->tail;
    }
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
            }
        }
    }
    else {
        type = (Type)malloc(sizeof(struct Type_));
        type->kind = BASIC;
        char *typename = specifier->child->val.c;
        if (strcmp(typename, "int") == 0) {
            type->u.basic = TYPE_INT;
        }
        else if (strcmp(typename, "float") == 0) {
            type->u.basic = TYPE_FLOAT;
        }
        else {
            printf("[DEBUG]Error TypeName: %s\n", typename);
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
            printf("Duplicate StructDef at Line %d\n", structSpecifier->lineno);
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
        Field field = (Field)malloc(sizeof(struct FieldList_));
        strcpy(field->name, sym->name);
        field->type = sym->u.type;
        if (structure == NULL) {
            structure = field;
            structure->tail = NULL;
        }
        else {
            if (addField(structure, field) < 0) {
                printf("DecMemVar Error at Line %d VarName: %s\n", dec->lineno, field->name);
            }
        }
        if (decList->subtype == UNFINISHED) {        
            decList = dec->sibling->sibling;
        }
        else {
            break;
        }
    }
    return buildFields(structure, def->sibling);
}

int addField(FieldList structure, Field field) {
    // structure is NOT NULL!
    FieldList cur_field = structure;
    while (cur_field != NULL) {
        if (strcmp(cur_field->name, field->name) == 0) {
            return -1;
        }
        if (cur_field->tail == NULL) {
            break;
        }
        cur_field = cur_field->tail;
    }
    cur_field->tail = field;
    field->tail = NULL;
    return 0;
}

Field getField(FieldList structure, char *name) {
    Field cur_field = structure;
    while (cur_field != NULL) {
        if (strcmp(cur_field->name, name) == 0) {
            return cur_field;
        }
        cur_field = cur_field->tail;
    }
    return NULL;
}

void decExtVar(Type type, ASTNode extDecList) {
    ASTNode varDec = extDecList->child;
    Symbol sym = getSym4VarDec(type, varDec);
    if (addIdDef(sym) < 0) {
        printf("DecExtVar Error at Line %d, VarName: %s\n", extDecList->lineno, sym->name);
    }
    if (extDecList->subtype == UNFINISHED) {
        decExtVar(type, varDec->sibling->sibling);
    }
}

void decVar(Type type, ASTNode decList) {
    ASTNode dec = decList->child;
    Symbol sym = getSym4VarDec(type, dec->child);
    if (addIdDef(sym) < 0) {
        printf("DecVar Error at Line %d, VarName: %s\n", decList->lineno, sym->name);
    }
    if (decList->subtype == UNFINISHED) {
        decVar(type, dec->sibling->sibling);
    }
}

void decFunc(Type type, ASTNode funDec) {
    Symbol sym = getSym4FuncDec(type, funDec);
    if (addIdDef(sym) < 0) {
        printf("DecFunc Error at Line %d, FuncName: %s\n", funDec->lineno, sym->name);
    }
}

Symbol getSym4VarDec(Type type, ASTNode varDec) {
    Symbol sym = (Symbol)malloc(sizeof(struct SymbolList_));
    if (varDec->subtype == TYPE_ARRAY) {
        Symbol subSym = getSym4VarDec(type, varDec->child);
        Type arr = (Type)malloc(sizeof(struct Type_));
        arr->kind = ARRAY;
        arr->u.array.elem = subSym->u.type;
        // TODO :size
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
        printf("DecParam Error at Line %d paramName: %s\n", varList->lineno, field->name);
    }
    if (addIdDef(sym) < 0) {
        printf("DecVar Error at Line %d varName: %s\n", varList->lineno, sym->name);
    }
    if (varList->subtype == UNFINISHED) {
        buildArgs(argList, paramDec->sibling->sibling);
    }
    return argList;
}

void semantic_parse(ASTNode parent) {
    if (parent == NULL) return;
    if (parent->type > AST_Program && parent->child == NULL) return;
    // child first
    for (ASTNode p = parent->child; p != NULL; p = p->sibling) {
        semantic_parse(p);
    }

    // /**/printf("sem_parse:%s\n", ASTNodeTypeName[parent->type]);
    Symbol func_sym = NULL;
    switch (parent->type) {
        case AST_ExtDef: {
            Type type = getType(parent->child);
            if (type == NULL) {
                printf("Undefined Type at Line %d\n", parent->lineno);
            }
            else if (parent->subtype == VAR_DEC) {
                decExtVar(type, parent->child->sibling);
            }
        }   break;
        case AST_Specifier: {
            Type type = getType(parent);
            if (type == NULL) {
                printf("Undefined Type at Line %d\n", parent->lineno);
            }
            else if (parent->subtype == FUNC_DEC) {
                decFunc(type, parent->sibling);
            }
        }   break;
        case AST_Def: {
            if (struct_env_dep > 0) break;
            Type type = getType(parent->child);
            if (type == NULL) {
                printf("Undefined Type at Line %d\n", parent->lineno);
            }
            else {
                decVar(type, parent->child->sibling);
            }
        }   break;
        case AST_LC: {
            if (parent->subtype == NEW_STRUCT) {
                struct_env_dep ++;
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
                    printf("Undefined Var: %s at Line %d\n", parent->val.c, parent-> lineno);
                }
            }
            else if(parent->subtype == FUNC_USE) {
                func_sym = getIdDef(parent->val.c);
                if (func_sym == NULL) {
                    printf("Undefined Id: %s at Line %d\n", parent->val.c, parent-> lineno);
                }
                else if (func_sym->kind != FUNC_DEF) {
                    printf("Id is not a Func: %s at Line %d\n", parent->val.c, parent-> lineno);
                }
                else {
                    checkArgs(func_sym->u.func->argList, parent->sibling->sibling);
                }
            }
        }   break;
        case AST_Exp:   checkExpType(parent); break;
        case AST_Stmt:  checkStmtType(parent);  break;

        // TODO
    }
}

Type checkExpType(ASTNode exp) { // TODO: duplicate check and report error
    Type type = NULL;
    ASTNode first = exp->child;
    switch (first->type) {
        case AST_Exp: {
            type = checkExpType(first);
            if (first->sibling->type == AST_ASSIGNOP) {
                if (isLeftVal(first) == 0) {
                    printf("Left Value Error at Line %d\n", exp->lineno);
                }
            }
            if (first->subtype == ARRAY_USE) {
                type = checkExpType(first);
                if (type != NULL) {
                    if (type->kind != ARRAY) {
                        printf("Not Array Type at Line %d\n", exp->lineno);
                        type = NULL;
                    }
                    else {
                        Type idx_type = checkExpType(first->sibling->sibling);
                        if (idx_type->kind != BASIC || idx_type->u.basic != TYPE_INT) {
                            printf("Index is Not a INT at Line %d\n", first->lineno);
                        }
                        type = type->u.array.elem;
                    }
                }
            }
            else if (first->subtype == STRUCT_USE) {
                Type fst_type = checkExpType(first);
                if (fst_type != NULL) {
                    if (fst_type->kind != STRUCTURE) {
                        printf("Not Struct Type at Line %d\n", exp->lineno);
                        type = NULL;
                    }
                    else {
                        Field field = getField(fst_type->u.structure, first->sibling->sibling->val.c);
                        if (field == NULL) {
                            printf("Non-exist field at Line %d\n", first->lineno);
                            type = NULL;
                        }
                        else {
                            type = field->type;
                        }
                    }
                }
            }
            else if (typeEqual(type, checkExpType(first->sibling->sibling)) == 0) {
                printf("Type mismatched at Line %d\n", exp->lineno);
                type = NULL;
            }
        }   break;
        case AST_ID: {
            Symbol sym = getIdDef(first->val.c);
            if (sym == NULL) {
                printf("Undefined Id at Line %d\n", first->lineno);
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

        // TODO
    }
    return type;
}

int isLeftVal(ASTNode exp) {
    ASTNode first = exp->child;
    switch (first->type) {
        case AST_Exp: {
            if (first->sibling->type == AST_LB || first->sibling->type == AST_DOT) {
                return 1;
            }
            return 0;
        }   break;
        case AST_LP:    return isLeftVal(first->sibling);   break;
        case AST_ID: {
            if (first->subtype == FUNC_USE) {
                return 0;
            }
            return 1;
        }   break;
        default:        return 0;
    }
    return 0;
}

int typeEqual(Type t1, Type t2) {
    if (t1 == NULL || t2 == NULL) return 0;
    if (t1->kind != t2->kind) return 0;
    if (t1->kind == BASIC) {
        return t1->u.basic == t2->u.basic;
    }
    return 0;
}

void checkStmtType(ASTNode stmt) {
    if (stmt->child->type == AST_RETURN) {
        Type retType = cur_func->u.func->retType;
        if (typeEqual(retType, checkExpType(stmt->child->sibling)) == 0) {
            printf("Type mismatched for RETURN at Line %d\n", stmt->lineno);
        }
    }
}

void checkArgs(FieldList argList, ASTNode args) {
    int matched = 1;
    if (args->child == NULL) {
        if (argList != NULL) {
            matched = 0;
        }
    }
    else {
        if (argList == NULL) {
            matched = 0;
        }
        else {
            matched = typeEqual(argList->type, checkExpType(args->child));
            if (matched) {
                if (args->subtype == UNFINISHED) {
                    checkArgs(argList->tail, args->child->sibling->sibling);
                    return;
                }
                else if (argList->tail != NULL) {
                    matched = 0;
                }
            }
        }
    }
    if (matched == 0) {
        printf("Function Args dismatched at Line %d\n", args->lineno);
    }
}