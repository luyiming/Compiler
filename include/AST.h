#ifndef AST_H
#define AST_H

enum ASTNodeType {
    AST_INT = 0,    AST_FLOAT,      AST_SEMI,
    AST_COMMA,      AST_ASSIGNOP,   AST_RELOP,
    AST_PLUS,       AST_MINUS,      AST_STAR,
    AST_DIV,        AST_AND,        AST_OR,
    AST_DOT,        AST_NOT,        AST_LP,
    AST_RP,         AST_LB,         AST_RB,
    AST_LC,         AST_RC,         AST_TYPE,
    AST_STRUCT,     AST_RETURN,     AST_IF,
    AST_ELSE,       AST_WHILE,      AST_ID,
    AST_Error,
    AST_Program,    AST_ExtDefList, AST_ExtDef,
    AST_ExtDecList, AST_Specifier,  AST_StructSpecifier,
    AST_OptTag,     AST_Tag,        AST_VarDec,
    AST_FunDec,     AST_VarList,    AST_ParamDec,
    AST_CompSt,     AST_StmtList,   AST_Stmt,
    AST_DefList,    AST_Def,        AST_DecList,
    AST_Dec,        AST_Exp,        AST_Args
};

#define MAX_TOKEN_SIZE 256

typedef struct Type_ *Type;

enum ASTNodeSubtype;

struct ASTNode_ {
    enum ASTNodeType type;
    int lineno;
    struct ASTNode_ *child, *sibling, *parent;
    union {
        int i;
        double d;
        char c[MAX_TOKEN_SIZE];
    } val;
    int subtype;    // for semantic parse, specify the production for derivation
    Type expType;   // only for AST_Exp
};
typedef struct ASTNode_ ASTNode;

ASTNode* newASTNode(enum ASTNodeType type, int lineno);

int addASTNode(ASTNode *parent, int count, ...);

void freeAST(ASTNode *parent);

void ASTwalk(ASTNode *parent, int indention);

extern ASTNode *ASTroot;

extern const char *const ASTNodeTypeName[];

#endif // AST_H
