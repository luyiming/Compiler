#ifndef __OC_H__
#define __OC_H__

#include "AST.h"
#include "ir.h"
#include "common.h"

typedef struct {
    enum { LV_TEMP, LV_VAR } kind;
    union {
        char *name;
        int id;
    } u;
    int off;        // offset to $fp
} LocalVarAddr;


typedef struct {
    char name[6];
    LocalVarAddr *lva;
    bool unused;
} Reg;

struct LvaList_ {
    LocalVarAddr *lva;
    struct LvaList_ *next;
};
typedef struct LvaList_ LvaList;

void generate_oc(ASTNode* Program);
void gen_data_seg();
void gen_global_seg();
void gen_text_seg(InterCodes* ics);
void gen_read_func();
void gen_write_func();
void init_reg();
Reg* get_reg(Operand* opd);
void free_reg(Reg* r);
void spill_reg(Reg* r);
LocalVarAddr* get_lva(Operand* opd);
LocalVarAddr* add_lva(Operand* opd);
void add_param2lva(Operand* opd);
void clear_lvas();
void gen_prologue();
void gen_epilogue();

#endif