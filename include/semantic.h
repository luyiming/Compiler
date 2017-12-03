#ifndef __SEMANTIC_H__
#define __SEMANTIC_H__ 1

#include "common.h"

#define MAX_NAME_SIZE 256

typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList, *Field;
struct Type_ {
    enum { BASIC, ARRAY, STRUCTURE } kind;
    union {
        enum { TYPE_INT, TYPE_FLOAT } basic;
        struct {
            Type elem;
            int size;
        } array;
        FieldList structure;
    } u;
};
struct FieldList_ {
    char name[MAX_NAME_SIZE];
    Type type;
    FieldList tail;
};

typedef struct Func_* Func;
struct Func_ {
    Type retType;
    FieldList argList;
    ASTNode definition;
    int lineno;
};

typedef struct SymbolList_* SymbolList, *Symbol;
struct SymbolList_ {
    enum { VAR_DEF, STRUCT_DEF, FUNC_DEF } kind;
    char name[MAX_NAME_SIZE];
    union {
        Type type;
        Func func;
    } u;
    SymbolList tail;
};

void        semantic_parse      (ASTNode ASTroot);

Symbol      loopupSymbol        (char* name, bool checkUpperScope);
Symbol      loopupType          (char* name);
int         insertSymbol        (Symbol sym);
int         insertType          (Symbol sym);

Type        getType             (ASTNode specifier);
Type        buildStructType     (ASTNode structSpecifier);
FieldList   buildFields         (FieldList structure, ASTNode defList);
int         addField            (FieldList structure, Field field);
Field       getField            (FieldList structure, char *name);
void        parseExtDecList     (Type type, ASTNode extDecList);
void        parseDecList        (Type type, ASTNode decList);
void        parseFunDec         (Type type, ASTNode funDec);
Symbol      getSym4VarDec       (Type type, ASTNode varDec);
Symbol      getSym4FunDec       (Type type, ASTNode funDec);

/**
 * @note   When building args for function declaration, there is no need to
 *         insert args into symbol table
 */
FieldList   buildArgs           (FieldList argList, ASTNode varList, bool addToSymbolTabel);

bool        isLeftVal           (ASTNode exp);
bool        typeEqual           (Type t1, Type t2);
bool        structEqual         (FieldList st1, FieldList st2);
bool        funcSignitureEqual  (Symbol func1, Symbol func2);
Type        checkExpType        (ASTNode exp);
void        checkStmtType       (ASTNode stmt);
void        checkArgs           (FieldList argList, ASTNode args);
void        checkUndefinedFunc  ();

/* ASTNode.subtype enum */
enum ASTNodeSubtype {
    DONTCARE = 0,   NEW_STRUCT,     TYPE_STRUCT,
    UNFINISHED,     TYPE_ARRAY,     EMPTY,
    VAR_USE,        FUNC_USE,       VOID_ARG,
    ARRAY_USE,      STRUCT_USE,     VAR_DEC,
    FUNC_DEC,       VOID_DEC,       INITIALIZE,
};

/* some constant */
#define     UNCHECKED       ((void*)-1)

#endif