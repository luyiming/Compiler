#ifndef __IR_H__
#define __IR_H__

#include "AST.h"
#include "semantic.h"

typedef struct {
    //todo: add OP_DEREF
    enum { OP_TEMP, OP_VARIABLE, OP_CONSTANT, OP_ADDR, OP_LABEL, OP_FUNCTION } kind;
    union {
        int var_id;
        int value;
        int label_id;
    } u;
    Symbol symbol;
} Operand;

#define VAR_NULL 0

typedef struct {
    enum {
        IR_LABEL,   // LABEL result :
        IR_FUNC,    // FUNCTION result :
        IR_ASSIGN,  // result := arg1
        IR_ADD,     // result := arg1 + arg2
        IR_SUB,     // result := arg1 - arg2
        IR_MUL,     // result := arg1 * arg2
        IR_DIV,     // result := arg1 / arg2

        IR_ADDR,    // result  := &arg1
        IR_DEREF_R, // result  := *arg1, dereference at right :-)
        IR_DEREF_L, // *result :=  arg1, dereference at left :-)

        IR_GOTO,    // GOTO result
        IR_RELOP,   // IF arg1 [relop] arg2 GOTO result
        IR_RETURN,  // RETURN result
        IR_DEC,     // DEC result [size]
        IR_ARG,     // ARG result
        IR_CALL,    // result := CALL arg1
        IR_PARAM,   // PARAM result

        IR_READ,    // READ result
        IR_WRITE    // WRITE result
    } kind;

    Operand result, arg1, arg2;

    enum RELOP_TYPE {
        RELOP_LT, RELOP_LE, RELOP_EQ,
        RELOP_GT, RELOP_GE, RELOP_NE
    } relop;
    int size;

} InterCode;

struct InterCodes_ {
    InterCode code;
    struct InterCodes_ *next, *prev;
};
typedef struct InterCodes_ InterCodes;

struct ArgNode_ {
    int var_id;
    struct ArgNode_ *next;
};
typedef struct ArgNode_ ArgNode;

InterCodes* concatInterCodes(int count, ...);

InterCodes* genLabelCode(int label_id);
InterCodes* genGotoCode(int label_id);

enum RELOP_TYPE get_relop(ASTNode *RELOP);

int getTypeSize(Type type);

InterCodes* translate_Exp(ASTNode *Exp, int place);
InterCodes* translate_Stmt(ASTNode *Stmt);
InterCodes* translate_StmtList(ASTNode *StmtList);
InterCodes* translate_CompSt(ASTNode *CompSt);
InterCodes* translate_Cond(ASTNode *Exp, int label_true, int label_false);
InterCodes* translate_Args(ASTNode *Args, ArgNode** arg_list);
InterCodes* translate_Program(ASTNode *Program);
InterCodes* translate_ExtDefList(ASTNode *ExtDefList);
InterCodes* translate_ExtDef(ASTNode *ExtDef);
InterCodes* translate_ExtDecList(ASTNode *ExtDecList);
InterCodes* translate_VarDec(ASTNode *VarDec);
InterCodes* translate_FunDec(ASTNode *FunDec);
InterCodes* translate_VarList(ASTNode *VarList);
InterCodes* translate_ParamDec(ASTNode *ParamDec);

InterCodes* translate_DefList(ASTNode *DefList);
InterCodes* translate_Def(ASTNode *Def);
InterCodes* translate_DecList(ASTNode *DecList);
InterCodes* translate_Dec(ASTNode *Dec);

void generate_ir(ASTNode* Program);

#define LABEL_FALL 0

#endif  // __IR_H__
