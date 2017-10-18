%{
#include <ctype.h>
#include "syntax.tab.h"

void debug(const char* type) {
    printf("%3d %-8s %s\n", yylineno, type, yytext);
}

int yycolumn = 1;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno

ws [ \t\r\n]+
relop ">"|"<"|">="|"<="|"=="|"!="

%%

(0|[1-9][0-9]*|0[0-7]+|0[Xx][0-9A-Fa-f]+)           { debug("INT"); yylval = atoi(yytext); return INT; }
([0-9]*\.[0-9]+|[0-9]+\.[0-9]*)([Ee][+-]?[0-9]+)?   { debug("FLOAT"); yylval = atof(yytext); return FLOAT; }
";"     { debug("SEMI"); return SEMI; }
","     { debug("COMMA"); return COMMA; }
"="     { debug("ASSIGNOP"); return ASSIGNOP; }
{relop} { debug("RELOP"); return RELOP; }
"+"     { debug("PLUS"); return PLUS; }
"-"     { debug("MINUS"); return MINUS; }
"*"     { debug("STAR"); return STAR; }
"/"     { debug("DIV"); return DIV; }
"&&"    { debug("AND"); return AND; }
"||"    { debug("OR"); return OR; }
"."     { debug("DOT"); return DOT; }
"!"     { debug("NOT"); return NOT; }
"("     { debug("LP"); return LP; }
")"     { debug("RP"); return RP; }
"["     { debug("LB"); return LB; }
"]"     { debug("RB"); return RB; }
"{"     { debug("LC"); return LC; }
"}"     { debug("RC"); return RC; }
"int"|"float"   { debug("TYPE"); return TYPE; }
"struct"        { debug("STRUCT"); return STRUCT; }
"return"        { debug("RETURN"); return RETURN; }
"if"            { debug("IF"); return IF; }
"else"          { debug("ELSE"); return ELSE; }
"while"         { debug("WHILE"); return WHILE; }
[_a-zA-Z][_a-zA-Z0-9]* { debug("ID"); return ID; }
\n { yycolumn = 1; }
{ws}+ { }

"//" {
    char c = input();
    while (c != '\n') c = input();
}

. {
    printf("Error type A at Line %d: Mysterious characters \"%s", yylineno, yytext);
    char c = input();
    while (!isspace(c)) {
        printf("%c", c);
        c = input();
    }
    printf("\"\n");
}

%%
