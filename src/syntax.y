%{
#include <stdio.h>
#include "AST.h"
#include "debug.h"
#include "semantic.h"
int yylex(void);
void yyerror(char*);
%}

%locations
%define api.value.type {ASTNode}
%destructor {
#if YYDEBUG == 1
    printf("free %s at Line %d\n", ASTNodeTypeName[$$->type], @$.first_line);
#endif
    freeAST($$);
} <>
%destructor { /* succeed */ } Program
 //TODO: Main Work : improve 'Error Recovery'
 //不同处理方法产生不同结果，有时意想不到，但是解释得通

/* declared tokens */
%token INT FLOAT
%token LC RC SEMI COMMA
%token TYPE STRUCT RETURN WHILE ID IF

%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT NEG
%left LP RP LB RB DOT

%%

/* High-level Definitions */
Program : ExtDefList                { $$ = ASTroot = newASTNode(AST_Program, @$.first_line); $$->child = ASTroot->child = $1; }
    ;

ExtDefList : ExtDef ExtDefList      { $$ = newASTNode(AST_ExtDefList, @$.first_line); addASTNode($$, 2, $1, $2); }
    | /* empty */                   { $$ = newASTNode(AST_ExtDefList, @$.first_line); }
    ;

ExtDef : Specifier ExtDecList SEMI  { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 3, $1, $2, $3); 
                                        decExtVar(getType($1), $2); }
    | Specifier SEMI                { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 2, $1, $2); 
                                        getType($1); }
    | Specifier FunDec CompSt       { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 3, $1, $2, $3);
                                        decFunc(getType($1), $2); }
    | error SEMI                    { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 2, newASTNode(AST_Error, @$.first_line), $2); yyerrok; }
    | Specifier error SEMI          { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 3, $1, newASTNode(AST_Error, @$.first_line), $3); yyerrok; }
    | Specifier error               { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 2, $1, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    | Specifier ExtDecList error    { $$ = newASTNode(AST_ExtDef, @$.first_line); addASTNode($$, 3, $1, $2, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    ;

ExtDecList : VarDec                 { $$ = newASTNode(AST_ExtDecList, @$.first_line); addASTNode($$, 1, $1); }
    | VarDec COMMA ExtDecList       { $$ = newASTNode(AST_ExtDecList, @$.first_line); $$->subtype = UNFINISHED; addASTNode($$, 3, $1, $2, $3); }
    | error COMMA ExtDecList        { $$ = newASTNode(AST_ExtDecList, @$.first_line); addASTNode($$, 3, newASTNode(AST_Error, @$.first_line), $2, $3); yyerrok; }
    | VarDec COMMA error            { $$ = newASTNode(AST_ExtDecList, @$.first_line); addASTNode($$, 3, $1, $2, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    ;

/* Specifiers */
Specifier : TYPE                    { $$ = newASTNode(AST_Specifier, @$.first_line); addASTNode($$, 1, $1); }
    | StructSpecifier               { $$ = newASTNode(AST_Specifier, @$.first_line); $$->subtype = TYPE_STRUCT; addASTNode($$, 1, $1); }
    ;

StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = newASTNode(AST_StructSpecifier, @$.first_line); $$->subtype = NEW_STRUCT; addASTNode($$, 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag                                { $$ = newASTNode(AST_StructSpecifier, @$.first_line); addASTNode($$, 2, $1, $2); }
    | STRUCT OptTag LC error RC                 { $$ = newASTNode(AST_StructSpecifier, @$.first_line); addASTNode($$, 5, $1, $2, $3, newASTNode(AST_Error, @$.first_line), $5); yyerrok; }
    ;

OptTag : ID                         { $$ = newASTNode(AST_OptTag, @$.first_line); addASTNode($$, 1, $1); }
    | /* empty */                   { $$ = newASTNode(AST_OptTag, @$.first_line); $$->subtype == EMPTY; }
    ;

Tag : ID                            { $$ = newASTNode(AST_Tag, @$.first_line); addASTNode($$, 1, $1); }
    ;

/* Declarators */
VarDec : ID                         { $$ = newASTNode(AST_VarDec, @$.first_line); addASTNode($$, 1, $1); }
    | VarDec LB INT RB              { $$ = newASTNode(AST_VarDec, @$.first_line); $$->subtype = TYPE_ARRAY; addASTNode($$, 4, $1, $2, $3, $4); }
    ;

FunDec : ID LP VarList RP           { $$ = newASTNode(AST_FunDec, @$.first_line); addASTNode($$, 4, $1, $2, $3, $4); }
    | ID LP RP                      { $$ = newASTNode(AST_FunDec, @$.first_line); $$->subtype = VOID_ARG; addASTNode($$, 3, $1, $2, $3); }
    | ID LP error RP                { $$ = newASTNode(AST_FunDec, @$.first_line); addASTNode($$, 4, $1, $2, newASTNode(AST_Error, @$.first_line), $4); yyerrok; }
    ;

VarList : ParamDec COMMA VarList    { $$ = newASTNode(AST_VarList, @$.first_line); $$->subtype = UNFINISHED; addASTNode($$, 3, $1, $2, $3); }
    | ParamDec                      { $$ = newASTNode(AST_VarList, @$.first_line); addASTNode($$, 1, $1); }
    ;

ParamDec : Specifier VarDec         { $$ = newASTNode(AST_ParamDec, @$.first_line); addASTNode($$, 2, $1, $2); }
    ;

/* Statements */
CompSt : LC DefList StmtList RC     { $$ = newASTNode(AST_CompSt, @$.first_line); addASTNode($$, 4, $1, $2, $3, $4); }
    ;

StmtList : Stmt StmtList            { $$ = newASTNode(AST_StmtList, @$.first_line); addASTNode($$, 2, $1, $2); }
    | /* empty */                   { $$ = newASTNode(AST_StmtList, @$.first_line); }
    ;

Stmt : Exp SEMI                                             { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 2, $1, $2); }
    | CompSt                                                { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 1, $1); }
    | RETURN Exp SEMI                                       { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | IF LP Exp RP Stmt             %prec LOWER_THEN_ELSE   { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt                           { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt                                  { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 5, $1, $2, $3, $4, $5); }
    | error SEMI                                            { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 2, newASTNode(AST_Error, @$.first_line), $2); yyerrok; }
    | Exp error                                             { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 2, $1, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    | RETURN Exp error                                      { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 3, $1, $2, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    | IF LP error RP Stmt             %prec LOWER_THEN_ELSE   { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 5, $1, $2, newASTNode(AST_Error, @$.first_line), $4, $5); yyerrok; }
    | IF LP error RP Stmt ELSE Stmt                           { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 7, $1, $2, newASTNode(AST_Error, @$.first_line), $4, $5, $6, $7); yyerrok; }
    | WHILE LP error RP Stmt                                  { $$ = newASTNode(AST_Stmt, @$.first_line); addASTNode($$, 5, $1, $2, newASTNode(AST_Error, @$.first_line), $4, $5); yyerrok; }
    ;

/* Local Definitions */
DefList : Def DefList               { $$ = newASTNode(AST_DefList, @$.first_line); addASTNode($$, 2, $1, $2); }
    | /* empty */                   { $$ = newASTNode(AST_DefList, @$.first_line); $$->subtype = EMPTY; }
    ;

Def : Specifier DecList SEMI        { decVar(getType($1), $2); $$ = newASTNode(AST_Def, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Specifier error SEMI          { $$ = newASTNode(AST_Def, @$.first_line); addASTNode($$, 3, $1, newASTNode(AST_Error, @$.first_line), $3); yyerrok; }
    | Specifier DecList error       { $$ = newASTNode(AST_Def, @$.first_line); addASTNode($$, 3, $1, $2, newASTNode(AST_Error, @$.first_line)); yyerrok; }
    ;

DecList : Dec                       { $$ = newASTNode(AST_DecList, @$.first_line); addASTNode($$, 1, $1); }
    | Dec COMMA DecList             { $$ = newASTNode(AST_DecList, @$.first_line); $$->subtype = UNFINISHED; addASTNode($$, 3, $1, $2, $3); }
    ;

Dec : VarDec                        { $$ = newASTNode(AST_Dec, @$.first_line); addASTNode($$, 1, $1); }
    | VarDec ASSIGNOP Exp           { $$ = newASTNode(AST_Dec, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp              { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp AND Exp                   { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp OR Exp                    { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp RELOP Exp                 { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp PLUS Exp                  { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp MINUS Exp                 { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp STAR Exp                  { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | Exp DIV Exp                   { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | LP Exp RP                     { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 3, $1, $2, $3); }
    | MINUS Exp                     { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 2, $1, $2); }
    | NOT Exp                       { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 2, $1, $2); }
    | ID LP Args RP                 { $$ = newASTNode(AST_Exp, @$.first_line); $1->subtype = FUNC_USE; addASTNode($$, 4, $1, $2, $3, $4); }
    | ID LP RP                      { $$ = newASTNode(AST_Exp, @$.first_line); $1->subtype = FUNC_USE; addASTNode($$, 3, $1, $2, $3); }
    | Exp LB Exp RB                 { $$ = newASTNode(AST_Exp, @$.first_line); $1->subtype = ARRAY_USE; addASTNode($$, 4, $1, $2, $3, $4); }
    | Exp DOT ID                    { $$ = newASTNode(AST_Exp, @$.first_line); $1->subtype = STRUCT_USE; addASTNode($$, 3, $1, $2, $3); }
    | ID                            { $$ = newASTNode(AST_Exp, @$.first_line); $1->subtype = VAR_USE; addASTNode($$, 1, $1); }
    | INT                           { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 1, $1); }
    | FLOAT                         { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 1, $1); }
    | Exp LB error RB               { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 4, $1, $2, newASTNode(AST_Error, @$.first_line), $4); yyerrok; }
    | ID LP error RP                { $$ = newASTNode(AST_Exp, @$.first_line); addASTNode($$, 4, $1, $2, newASTNode(AST_Error, @$.first_line), $4); yyerrok; }
    ;

Args : Exp COMMA Args               { $$ = newASTNode(AST_Args, @$.first_line); $$->subtype = UNFINISHED; addASTNode($$, 3, $1, $2, $3); }
    | Exp                           { $$ = newASTNode(AST_Args, @$.first_line); addASTNode($$, 1, $1); }
    ;

%%

#include "lex.yy.c"

void yyerror(char *msg) {
  reportError('B', "%s", msg);
}
