#include <stdio.h>
#include <string.h>
#include "AST.h"
#include "debug.h"

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

    ASTwalk(ASTroot, 0);

    fclose(fin);
    if (argc == 3) {
        fclose(fout);
    }

    return 0;
}
