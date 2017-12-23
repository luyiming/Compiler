#include <stdio.h>
#include <string.h>
#include "AST.h"
#include "debug.h"
#include "ir.h"
#include "sym_table.h"

#if YYDEBUG == 1
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
    if (!fin) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(fin);

#if YYDEBUG == 1
    yydebug = 1;
#endif

    yyparse();

    // ASTwalk(ASTroot, 0);

    initSymbolTabel();

    Symbol read_func = (Symbol)malloc(sizeof(struct SymbolList_));
    strcpy(read_func->name, "read");
    read_func->kind = FUNC_DEF;
    read_func->u.func = (Func)malloc(sizeof(struct Func_));
    read_func->u.func->definition = (ASTNode*)malloc(sizeof(ASTNode)); // to make checkUndefinedFunc() happy
    insertSymbol(read_func);

    Symbol write_func = (Symbol)malloc(sizeof(struct SymbolList_));
    strcpy(write_func->name, "write");
    write_func->kind = FUNC_DEF;
    write_func->u.func = (Func)malloc(sizeof(struct Func_));
    write_func->u.func->definition = (ASTNode*)malloc(sizeof(ASTNode)); // to make checkUndefinedFunc() happy
    insertSymbol(write_func);

    semantic_parse(ASTroot);

    generate_ir(ASTroot);

    freeAST(ASTroot);

    return 0;
}
