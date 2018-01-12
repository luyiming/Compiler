#include <stdio.h>
#include <string.h>
#include "AST.h"
#include "debug.h"
#include "ir.h"
#include "oc.h"
#include "sym_table.h"

#ifdef YYDEBUG
extern int yydebug;
#endif

extern FILE *yyin;
extern int yylex(void);
extern void yyrestart(FILE *);
extern int yyparse(void);
extern ASTNode *ASTroot;

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    FILE *fin = fopen(argv[1], "r");
    FILE *fout = NULL;
    if (!fin) {
        perror(argv[1]);
        return 1;
    }

    if (argc == 3) {
        fout = freopen(argv[2], "w", stdout);
    }

    yyrestart(fin);

#ifdef YYDEBUG
    yydebug = 1;
#endif

    yyparse();

    // ASTwalk(ASTroot, 0);

    initSymbolTabel();

    Symbol read_func = (Symbol)malloc(sizeof(struct SymbolList_));
    memset(read_func, 0, sizeof(struct SymbolList_));
    strcpy(read_func->name, "read");
    read_func->kind = FUNC_DEF;
    read_func->tail = NULL;
    read_func->u.func = (Func)malloc(sizeof(struct Func_));
    memset(read_func->u.func, 0, sizeof(struct Func_));
    read_func->u.func->definition = (ASTNode*)malloc(sizeof(ASTNode)); // to make checkUndefinedFunc() happy
    memset(read_func->u.func->definition, 0, sizeof(ASTNode));
    read_func->u.func->retType = (Type)malloc(sizeof(struct Type_)); // use in semantic.c - typeEqual(Type t1, Type t2)
    memset(read_func->u.func->retType, 0, sizeof(struct Type_));
    read_func->u.func->retType->kind = BASIC;
    read_func->u.func->retType->u.basic = TYPE_INT;
    insertSymbol(read_func);

    Symbol write_func = (Symbol)malloc(sizeof(struct SymbolList_));
    memset(write_func, 0, sizeof(struct SymbolList_));
    strcpy(write_func->name, "write");
    write_func->kind = FUNC_DEF;
    write_func->u.func = (Func)malloc(sizeof(struct Func_));
    memset(write_func->u.func, 0, sizeof(struct Func_));
    write_func->u.func->definition = (ASTNode*)malloc(sizeof(ASTNode)); // to make checkUndefinedFunc() happy
    memset(write_func->u.func->definition, 0, sizeof(ASTNode));
    insertSymbol(write_func);

    semantic_parse(ASTroot);

    // generate_ir(ASTroot);
    // printf("---------------------------\n");
    generate_oc(ASTroot);

    freeAST(ASTroot);

    fclose(fin);
    if (argc == 3) {
        fclose(fout);
    }

    return 0;
}
