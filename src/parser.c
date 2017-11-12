#include <stdio.h>
#include "../include/AST.h"
#include "../include/debug.h"
#if YYDEBUG == 1
  extern int yydebug;
#endif

extern FILE *yyin;
extern int yylex(void);
extern void yyrestart(FILE*);
extern int yyparse(void);
extern struct ASTNode *ASTroot;
extern int cnt_error;

int main(int argc, char **argv)
{
    if (argc <= 1)
        return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
  #if YYDEBUG == 1
    yydebug = 1;
  #endif
    yyparse();

    if (cnt_error == 0) {
      ASTwalk(ASTroot, 0);
    }
    else {
    #ifdef ERROR_AST
      ASTwalk(ASTroot, 0);
    #endif
    }
    freeAST(ASTroot);
    ASTroot = NULL;

    return 0;
}
