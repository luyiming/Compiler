#ifndef __SEMANTIC_H__
#define __SEMANTIC_H__ 1

#define NAME_SIZE 100
typedef struct Type_* Type;
typedef struct FieldList_* FieldList, *Field;
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
    char name[NAME_SIZE];
    Type type;
    FieldList tail;
};

typedef struct Func_* Func;
struct Func_ {
    Type retType;
    FieldList argList;
};

typedef struct SymbolList_* SymbolList, *Symbol;
struct SymbolList_ {
    enum { VAR_DEF, STRUCT_DEF, FUNC_DEF } kind;
    char name[NAME_SIZE];
    union {
        Type type;
        Func func;
    } u;
    SymbolList tail;
};
Symbol getIdDef(char *name);
Symbol getTypeDef(char *name);
int addIdDef(Symbol sym);
int addTypeDef(Symbol sym);

Type getType(ASTNode specifier);
Type newType(ASTNode structSpecifier);
FieldList buildFields(FieldList structure, ASTNode defList);
int addField(FieldList structure, Field field);
Field getField(FieldList structure, char *name);
void decExtVar(Type type, ASTNode extDecList);
void decVar(Type type, ASTNode decList);
void decFunc(Type type, ASTNode funDec);
Symbol getSym4VarDec(Type type, ASTNode dec);
Symbol getSym4FuncDec(Type type, ASTNode funDec);
FieldList buildArgs(FieldList argList, ASTNode varList);

void semantic_parse(ASTNode ASTroot);
Type checkExpType(ASTNode exp);
int isLeftVal(ASTNode exp);
int typeEqual(Type t1, Type t2);
int structEqual(FieldList st1, FieldList st2);
void checkStmtType(ASTNode stmt);
void checkArgs(FieldList argList, ASTNode args);

/* ASTNode.subtype enum */
enum ASTNodeSubtype {
    DONTCARE = 0,   NEW_STRUCT,     TYPE_STRUCT,
    UNFINISHED,     TYPE_ARRAY,     EMPTY,
    VAR_USE,        FUNC_USE,       VOID_ARG,
    ARRAY_USE,      STRUCT_USE,     VAR_DEC,
    FUNC_DEC,       VOID_DEC,       
};

/* some constant */
#define     UNCHECKED       ((void*)-1)

#endif