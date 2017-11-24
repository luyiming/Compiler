#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "AST.h"

struct ASTNode *ASTroot = NULL;

const char *const ASTNodeTypeName[] = {
    "INT",        "FLOAT",      "SEMI",
    "COMMA",      "ASSIGNOP",   "RELOP",
    "PLUS",       "MINUS",      "STAR",
    "DIV",        "AND",        "OR",
    "DOT",        "NOT",        "LP",
    "RP",         "LB",         "RB",
    "LC",         "RC",         "TYPE",
    "STRUCT",     "RETURN",     "IF",
    "ELSE",       "WHILE",      "ID",
  //--- terminators above
    "Error",
    "Program",    "ExtDefList", "ExtDef",
    "ExtDecList", "Specifier",  "StructSpecifier",
    "OptTag",     "Tag",        "VarDec",
    "FunDec",     "VarList",    "ParamDec",
    "CompSt",     "StmtList",   "Stmt",
    "DefList",    "Def",        "DecList",
    "Dec",        "Exp",        "Args"
};

struct ASTNode *newASTNode(enum ASTNodeType type, int lineno) {
    struct ASTNode *p = (struct ASTNode *)malloc(sizeof(struct ASTNode));
    p->child = p->sibling = NULL;
    p->type = type;
    p->lineno = lineno;
    return p;
}

int addASTNode(struct ASTNode *parent, int count, ...) {
    if (count < 1)
        return -1;

    va_list argp;
    va_start(argp, count);

    struct ASTNode *p, *q;
    parent->child = p = va_arg(argp, struct ASTNode *);
    for (int i = 1; i < count; i++) {
        q = va_arg(argp, struct ASTNode *);
        p->sibling = q;
        p = q;
    }

    va_end(argp);
    return 0;
}

void ASTwalk(struct ASTNode *parent, int indention) {
    if (parent == NULL)
        return;

    if (parent->type > AST_Program && parent->child == NULL)
        return;

    for (int i = 0; i < indention * 2; i++) {
        printf(" ");
    }
    printf("%s", ASTNodeTypeName[parent->type]);

    switch(parent->type) {
        case AST_TYPE: case AST_ID:
            printf(": %s\n", parent->val.c); break;
        case AST_INT:
            printf(": %d\n", parent->val.i); break;
        case AST_FLOAT:
            printf(": %lf\n", parent->val.d); break;

        case AST_Program:    case AST_ExtDefList: case AST_ExtDef:
        case AST_ExtDecList: case AST_Specifier:  case AST_StructSpecifier:
        case AST_OptTag:     case AST_Tag:        case AST_VarDec:
        case AST_FunDec:     case AST_VarList:    case AST_ParamDec:
        case AST_CompSt:     case AST_StmtList:   case AST_Stmt:
        case AST_DefList:    case AST_Def:        case AST_DecList:
        case AST_Dec:        case AST_Exp:        case AST_Args:
        case AST_Error:
            printf(" (%d)\n", parent->lineno); break;

        default : printf("\n");
    }

    for (struct ASTNode *p = parent->child; p != NULL; p = p->sibling) {
        ASTwalk(p, indention + 1);
    }
}

void freeAST(struct ASTNode* parent) {
    if (parent == NULL)
        return;

    for (struct ASTNode *p = parent->child; p != NULL; p = p->sibling) {
        freeAST(p);
    }

    free(parent);
}
