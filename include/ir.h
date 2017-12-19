#ifndef __IR_H__
#define __IR_H__

#include "AST.h"

typedef struct {
    enum { VARIABLE, CONSTANT, ADDRESS } kind;
    union {
        int var_no;
        int value;
    } u;
} Operand;

typedef struct {
    enum {
        IR_LABEL,   // LABEL x :
        IR_FUNC,    // FUNCTION f :
        IR_ASSIGN,  // x := y
        IR_ADD,     // x := y + z
        IR_SUB,     // x := y - z
        IR_MUL,     // x := y * z

        IR_REF,    // x := &y
        IR_DEREF,   // x := *y
        IR_A_ADDR,  // *x := y

        IR_GOTO,    // GOTO x
        IR_RELOP,   // IF x [relop] y GOTO z
        IR_RETURN,  // RETURN x
        IR_DEC,     // DEC x [size]
        IR_ARG,     // ARG x
        IR_CALL,    // x := CALL f
        IR_PARAM    // PARAM x
    } kind;
    union {
        struct {
            Operand right, left;
        } assign;
        struct {
            Operand result, op1, op2;
        } binop;
    } u;
} InterCode;

struct InterCodes_ {
    InterCode code;
    struct InterCodes_ *next, *prev;
};
typedef struct InterCodes_ InterCodes;

InterCodes* concatInterCodes(int count, ...);

InterCodes* translate_Exp(ASTNode *exp);

#endif  // __IR_H__
