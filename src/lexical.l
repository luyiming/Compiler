%{
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "syntax.tab.h"
#include "AST.h"
#include "debug.h"
#include "common.h"

void debug(const char* type) {
#ifdef LEXDEBUG
  printf("%3d %-8s %s\n", yylineno, type, yytext);
#endif
}
int str2i(const char* str) {
  int res;
  if (str[0] == '0') {
    if (str[1] == 'x' || str[1] == 'X') {
      sscanf(str + 2, "%x", &res);
    }
    else if (str[1] != '\0') {
      sscanf(str + 1, "%o", &res);
    }
    else {
      res = 0;
    }
  }
  else {
    sscanf(str, "%d", &res);
  }
  return res;
}

int yycolumn = 1;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno

ws      [ \t\r\n]+
relop   ">"|"<"|">="|"<="|"=="|"!="
int     (0|[1-9][0-9]*|0[0-7]+|0[Xx][0-9A-Fa-f]+)
float   ([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([Ee][+-]?[0-9]+)?
id      [_a-zA-Z][_a-zA-Z0-9]*

%x COMMENT
%%

{int}           { debug("INT");      yylval = newASTNode(AST_INT, yylloc.first_line);       yylval->val.i = str2i(yytext); return INT; }
{float}         { debug("FLOAT");    yylval = newASTNode(AST_FLOAT, yylloc.first_line);     yylval->val.d = atof(yytext);  return FLOAT; }
";"             { debug("SEMI");     yylval = newASTNode(AST_SEMI, yylloc.first_line);      return SEMI; }
","             { debug("COMMA");    yylval = newASTNode(AST_COMMA, yylloc.first_line);     return COMMA; }
"="             { debug("ASSIGNOP"); yylval = newASTNode(AST_ASSIGNOP, yylloc.first_line);  return ASSIGNOP; }
{relop}         { debug("RELOP");    yylval = newASTNode(AST_RELOP, yylloc.first_line);     strcpy(yylval->val.c, yytext); return RELOP; }
"+"             { debug("PLUS");     yylval = newASTNode(AST_PLUS, yylloc.first_line);      return PLUS; }
"-"             { debug("MINUS");    yylval = newASTNode(AST_MINUS, yylloc.first_line);     return MINUS; }
"*"             { debug("STAR");     yylval = newASTNode(AST_STAR, yylloc.first_line);      return STAR; }
"//".*          { debug("LCOMMENT"); }
"/*"            { debug("BCOMMENT"); BEGIN(COMMENT); }
<COMMENT>"*/"   { BEGIN(INITIAL); }
<COMMENT>.|\n   { }
<COMMENT><<EOF>>    { reportError("A", yylineno, "unterminated comment"); BEGIN(INITIAL); }
"/"             { debug("DIV");      yylval = newASTNode(AST_DIV, yylloc.first_line);       return DIV; }
"&&"            { debug("AND");      yylval = newASTNode(AST_AND, yylloc.first_line);       return AND; }
"||"            { debug("OR");       yylval = newASTNode(AST_OR, yylloc.first_line);        return OR; }
"."             { debug("DOT");      yylval = newASTNode(AST_DOT, yylloc.first_line);       return DOT; }
"!"             { debug("NOT");      yylval = newASTNode(AST_NOT, yylloc.first_line);       return NOT; }
"("             { debug("LP");       yylval = newASTNode(AST_LP, yylloc.first_line);        return LP; }
")"             { debug("RP");       yylval = newASTNode(AST_RP, yylloc.first_line);        return RP; }
"["             { debug("LB");       yylval = newASTNode(AST_LB, yylloc.first_line);        return LB; }
"]"             { debug("RB");       yylval = newASTNode(AST_RB, yylloc.first_line);        return RB; }
"{"             { debug("LC");       yylval = newASTNode(AST_LC, yylloc.first_line);        return LC; }
"}"             { debug("RC");       yylval = newASTNode(AST_RC, yylloc.first_line);        return RC; }
"int"|"float"   { debug("TYPE");     yylval = newASTNode(AST_TYPE, yylloc.first_line);      strcpy(yylval->val.c, yytext); return TYPE; }
"struct"        { debug("STRUCT");   yylval = newASTNode(AST_STRUCT, yylloc.first_line);    return STRUCT; }
"return"        { debug("RETURN");   yylval = newASTNode(AST_RETURN, yylloc.first_line);    return RETURN; }
"if"            { debug("IF");       yylval = newASTNode(AST_IF, yylloc.first_line);        return IF; }
"else"          { debug("ELSE");     yylval = newASTNode(AST_ELSE, yylloc.first_line);      return ELSE; }
"while"         { debug("WHILE");    yylval = newASTNode(AST_WHILE, yylloc.first_line);     return WHILE; }
{id}            { debug("ID");       yylval = newASTNode(AST_ID, yylloc.first_line);        strcpy(yylval->val.c, yytext); return ID; }
\r\n|\n         { yycolumn = 1; }
{ws}+           { }

.               { reportError("A", yylineno, "mysterious character \'%s\'", yytext); }

%%
