#include <stdio.h>
#include "AST.h"
#include "debug.h"
#include "ir.h"

#if YYDEBUG == 1
extern int yydebug;
#endif

extern FILE *yyin;
extern int yylex(void);
extern void yyrestart(FILE *);
extern int yyparse(void);
extern ASTNode *ASTroot;

int main(int argc, char **argv) {
    if (argc <= 2) return 1;
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

    gen_ir(ASTroot);

    freeAST(ASTroot);

    return 0;
}
