#include "oc.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "debug.h"

#define println(format, ...) printf(format "\n", ## __VA_ARGS__)
#define printIns(format, ...) printf("  " format "\n", ## __VA_ARGS__)

LvaList *lva_list = NULL;
int lva_off = 0, param_off = 0;
Reg t_regs[10];

void generate_oc(ASTNode* program) {
    gen_data_seg();
    gen_global_seg();
    InterCodes *ics = translate_Program(program);
    gen_text_seg(ics);
}

void gen_data_seg() {
    println(".data");
    println("_prompt: .asciiz \"Enter an integer:\"");
    println("_ret: .asciiz \"\\n\"");
}

void gen_global_seg() {
    println(".globl main");
}

void gen_read_func() {
    println();
    println("read:");
    printIns("li $v0, 4");
    printIns("la $a0, _prompt");
    printIns("syscall");
    printIns("li $v0, 5");
    printIns("syscall");
    printIns("jr $ra");
}

void gen_write_func() {
    println();
    println("write:");
    printIns("li $v0, 1");
    printIns("syscall");
    printIns("li $v0, 4");
    printIns("la $a0, _ret");
    printIns("syscall");
    printIns("move $v0, $0");
    printIns("jr $ra");
}

void gen_text_seg(InterCodes* ics) {
    init_reg();
    println(".text");
    gen_read_func();
    gen_write_func();
    InterCodes* ic = ics;
    for (; ic != NULL; ic = ic->next) {
        switch (ic->code.kind) {
            case IR_LABEL: {
                println("label%d:", ic->code.result.u.label_id);
                break;
            }
            case IR_FUNC: {
                println();
                println("%s:", ic->code.result.symbol->name);
                clear_lvas();
                gen_prologue();
                break;
            }
            case IR_ASSIGN: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                printIns("move %s, %s", rr->name, r1->name);
                spill_reg(rr);
                free_reg(r1);
                break;
            }
            case IR_ADD: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                Reg* r2 = get_reg(&ic->code.arg2);
                printIns("add %s, %s, %s", rr->name, r1->name, r2->name);
                spill_reg(rr);
                free_reg(r1);
                free_reg(r2);
                break;
            }
            case IR_SUB: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                Reg* r2 = get_reg(&ic->code.arg2);
                printIns("sub %s, %s, %s", rr->name, r1->name, r2->name);
                spill_reg(rr);
                free_reg(r1);
                free_reg(r2);
                break;
            }
            case IR_MUL: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                Reg* r2 = get_reg(&ic->code.arg2);
                printIns("mul %s, %s, %s", rr->name, r1->name, r2->name);
                spill_reg(rr);
                free_reg(r1);
                free_reg(r2);
                break;
            }
            case IR_DIV: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                Reg* r2 = get_reg(&ic->code.arg2);
                printIns("div %s, %s", r1->name, r2->name);
                printIns("mflo %s", rr->name);
                spill_reg(rr);
                free_reg(r1);
                free_reg(r2);
                break;
            }
            case IR_ADDR: {
                Reg* rr = get_reg(&ic->code.result);
                gen_addr(rr, &ic->code.arg1);
                spill_reg(rr);
                break;
            }
            case IR_DEREF_L: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                printIns("sw %s, 0(%s)", r1->name, rr->name);
                free_reg(rr);
                free_reg(r1);
                break;
            }
            case IR_DEREF_R: {
                Reg* rr = get_reg(&ic->code.result);
                Reg* r1 = get_reg(&ic->code.arg1);
                printIns("lw %s, 0(%s)", rr->name, r1->name);
                spill_reg(rr);
                free_reg(r1);
                break;
            }
            case IR_GOTO: {
                printIns("j label%d", ic->code.result.u.label_id);
                break;
            }
            case IR_RELOP: {
                char *ins;
                switch (ic->code.relop) {
                    case RELOP_EQ: ins = "beq"; break;
                    case RELOP_NE: ins = "bne"; break;
                    case RELOP_GT: ins = "bgt"; break;
                    case RELOP_LT: ins = "blt"; break;
                    case RELOP_GE: ins = "bge"; break;
                    case RELOP_LE: ins = "ble"; break;
                    default: assert(0);
                }
                Reg* r1 = get_reg(&ic->code.arg1);
                Reg* r2 = get_reg(&ic->code.arg2);
                printIns("%s %s, %s, label%d", ins, r1->name, r2->name,
                    ic->code.result.u.label_id);
                free_reg(r1);
                free_reg(r2);
                break;
            }
            case IR_RETURN: {
                Reg* rr = get_reg(&ic->code.result);
                printIns("move $v0, %s", rr->name);
                // invoke gen_epilogue() after get_reg(...)
                // avoid to reset $fp early
                gen_epilogue();
                printIns("jr $ra");
                free_reg(rr);
                break;
            }
            case IR_DEC: {
                add_array2lva(&ic->code.result, ic->code.size);
                break;
            }
            case IR_ARG: {
                // put all args on stack
                Reg* rr = get_reg(&ic->code.result);
                printIns("addi $sp, $sp, -4");
                printIns("sw %s, 0($sp)", rr->name);
                free_reg(rr);
                break;
            }
            case IR_CALL: {
                printIns("addi $sp, $sp, -4");
                printIns("sw $ra, 0($sp)");
                printIns("jal %s", ic->code.arg1.symbol->name);
                printIns("lw $ra, 0($sp)");
                printIns("addi $sp, $sp, 4");
                // don't invoke get_reg(...) before 'jal ...'
                // avoid to allocate stack between ARG... and CALL...
                Reg* rr = get_reg(&ic->code.result);
                printIns("move %s, $v0", rr->name);
                spill_reg(rr);
                break;
            }
            case IR_PARAM: {
                add_param2lva(&ic->code.result);
                break;
            }
            case IR_READ: {
                Reg* rr = get_reg(&ic->code.result);
                printIns("addi $sp, $sp, -4");
                printIns("sw $ra, 0($sp)");
                printIns("jal read");
                printIns("lw $ra, 0($sp)");
                printIns("addi $sp, $sp, 4");
                printIns("move %s, $v0", rr->name);
                spill_reg(rr);
                break;
            }
            case IR_WRITE: {
                Reg* rr = get_reg(&ic->code.result);
                printIns("move $a0, %s", rr->name);
                free_reg(rr);

                printIns("addi $sp, $sp, -4");
                printIns("sw $ra, 0($sp)");
                printIns("jal write");
                printIns("lw $ra, 0($sp)");
                printIns("addi $sp, $sp, 4");
                break;
            }
            default: assert(0);
        }
    }
}

Reg* get_reg(Operand* opd) {
    for (int i = 0; i < 10; i ++) {
        if (t_regs[i].unused) {
            t_regs[i].unused = false;
            if (opd->kind == OP_CONSTANT) {
                printIns("li %s, %d", t_regs[i].name, opd->u.value);
            }
            else {
                t_regs[i].lva = get_lva(opd);
                printIns("lw %s, %d($fp)", t_regs[i].name, t_regs[i].lva->off);
            }
            return t_regs + i;
        }
    }
    panic();
}

void spill_reg(Reg* r) {
    int off = r->lva->off;
    printIns("sw %s, %d($fp)", r->name, off);
    free_reg(r);
}

void free_reg(Reg* r) {
    r->unused = true;
    r->lva = NULL;
}

void init_reg() {
    for (int i = 0; i < 10; i ++) {
        t_regs[i].name[0] = '$';
        t_regs[i].name[1] = 't';
        t_regs[i].name[2] = '0' + i;
        t_regs[i].unused = true;
        t_regs[i].lva = NULL;
    }
}

LocalVarAddr* get_lva(Operand* opd) {
    LvaList *node = lva_list;
    for (; node != NULL; node = node->next) {
        if ((opd->kind == OP_TEMP && node->lva->kind == LV_TEMP
            && opd->u.var_id == node->lva->u.id)
        || (opd->kind == OP_VARIABLE && node->lva->kind == LV_VAR
            && strcmp(opd->symbol->name, node->lva->u.name) == 0)) {
            return node->lva;
        }
    }
    return add_lva(opd);        
}

LocalVarAddr* add_lva(Operand* opd) {
    LocalVarAddr* lva = (LocalVarAddr*)malloc(sizeof(LocalVarAddr));
    if (opd->kind == OP_TEMP) {
        lva->kind = LV_TEMP;
        lva->u.id = opd->u.var_id;
    } else if (opd->kind == OP_VARIABLE) {
        lva->kind = LV_VAR;
        lva->u.name = opd->symbol->name;
    } else
        assert(0);
    lva->off = (lva_off -= 4);
    LvaList *node = (LvaList*)malloc(sizeof(LvaList));
    node->lva = lva;
    node->next = lva_list;
    lva_list = node;
    printIns("addi $sp, $sp, -4"); // allocate
    return lva;
}

void add_array2lva(Operand* opd, int size) {
    assert(opd->kind == OP_TEMP);
    LocalVarAddr* lva = (LocalVarAddr*)malloc(sizeof(LocalVarAddr));
    lva->kind = LV_TEMP;
    lva->u.id = opd->u.var_id;
    lva->off = (lva_off -= size);
    LvaList *node = (LvaList*)malloc(sizeof(LvaList));
    node->lva = lva;
    node->next = lva_list;
    lva_list = node;
    printIns("addi $sp, $sp, -%d", size); // allocate
}

void add_param2lva(Operand* opd) {
    assert(opd->kind == OP_VARIABLE);
    LocalVarAddr* lva = (LocalVarAddr*)malloc(sizeof(LocalVarAddr));
    lva->kind = LV_VAR;
    lva->u.name = opd->symbol->name;
    lva->off = (param_off += 4);
    LvaList *node = (LvaList*)malloc(sizeof(LvaList));
    node->lva = lva;
    node->next = lva_list;
    lva_list = node;
}

void clear_lvas() {
    lva_list = NULL;
    lva_off = 0;
    param_off = 4;
}

void gen_prologue() {
    printIns("addi $sp, $sp, -4");
    printIns("sw $fp, 0($sp)");
    printIns("move $fp, $sp");
}

void gen_epilogue() {
    printIns("move $sp, $fp");
    printIns("lw $fp, 0($sp)");
    printIns("addi $sp, $sp, 4");
}

void gen_addr(Reg* r, Operand* opd) {
    LocalVarAddr* lva = get_lva(opd);
    printIns("la %s, %d($fp)", r->name, lva->off);
}

InterCodes* ic_start, *ic_end;

void peek_basic_block2(InterCodes* codes) {
    for(ic_start = codes; ic_start != NULL; ic_start = ic_start->next) {
        if (ic_start->code.kind == IR_ASSIGN || ic_start->code.kind == IR_ADD || 
            ic_start->code.kind == IR_SUB  || ic_start->code.kind == IR_MUL ||
            ic_start->code.kind == IR_DIV || ic_start->code.kind == IR_ARG) {
                break;
            }
    }

    for(ic_end = ic_start; ic_end != NULL; ic_end = ic_end->next) {
        if (ic_end->code.kind != IR_ASSIGN && ic_end->code.kind != IR_ADD && 
            ic_end->code.kind != IR_SUB  && ic_end->code.kind != IR_MUL &&
            ic_end->code.kind != IR_DIV && ic_end->code.kind != IR_ARG &&
            ic_end->code.kind != IR_DEREF_L && ic_end->code.kind != IR_DEREF_R && ic_end->code.kind != IR_ADDR) {
                ic_end = ic_end->next;
                break;
            }
    }
}

Reg* alloc_reg(Operand* opd) {
    Reg* reg = NULL;
    for (InterCodes* p = ic_end; p != ic_start; p = p->prev) {
        for (int i = 9; i >= 0; i--) {
            Reg* r = t_regs + i;
            if ((p->code.arg1.kind == OP_TEMP && p->code.arg1.u.label_id == r->lva->u.id) ||
                (p->code.arg1.kind == OP_VARIABLE && strcmp(p->code.arg1.symbol->name, r->name) == 0)) {
                    reg = r;
                    goto end;
                }
            if ((p->code.arg2.kind == OP_TEMP && p->code.arg2.u.label_id == r->lva->u.id) ||
                (p->code.arg2.kind == OP_VARIABLE && strcmp(p->code.arg2.symbol->name, r->name) == 0)) {
                    reg = r;
                    goto end;
                }
            if ((p->code.result.kind == OP_TEMP && p->code.result.u.label_id == r->lva->u.id) ||
                (p->code.result.kind == OP_VARIABLE && strcmp(p->code.result.symbol->name, r->name) == 0)) {
                    reg = r;
                    goto end;
                }
        }
    }
    end:
    spill_reg(reg);
    return reg;
}

Reg* get_reg2(Operand* opd) {
    for (int i = 0; i < 10; i ++) {
        if (t_regs[i].unused) {
            t_regs[i].unused = false;
            if (opd->kind == OP_CONSTANT) {
                printIns("li %s, %d", t_regs[i].name, opd->u.value);
            }
            else {
                t_regs[i].lva = get_lva(opd);
                printIns("lw %s, %d($fp)", t_regs[i].name, t_regs[i].lva->off);
            }
            return t_regs + i;
        }
    }
    return alloc_reg(opd);
}

void try_free_reg(InterCodes* current, Reg* r) {
    bool can_free = true;
    for (InterCodes* p = current + 1; p != ic_end; p = p->next) {
        if ((p->code.arg1.kind == OP_TEMP && p->code.arg1.u.label_id == r->lva->u.id) ||
            (p->code.arg1.kind == OP_VARIABLE && strcmp(p->code.arg1.symbol->name, r->name) == 0)) {
                can_free = false;
                break;
            }
        if ((p->code.arg2.kind == OP_TEMP && p->code.arg2.u.label_id == r->lva->u.id) ||
            (p->code.arg2.kind == OP_VARIABLE && strcmp(p->code.arg2.symbol->name, r->name) == 0)) {
                can_free = false;
                break;
            }
        if ((p->code.result.kind == OP_TEMP && p->code.result.u.label_id == r->lva->u.id) ||
            (p->code.result.kind == OP_VARIABLE && strcmp(p->code.result.symbol->name, r->name) == 0)) {
                can_free = false;
                break;
            }
    }
    if (can_free) {
        r->unused = true;
        r->lva = NULL;
    }
}

void spill_all_regs() {
    for (int i = 0; i < 10; i ++) {
        if (!t_regs[i].unused) {
            spill_reg(t_regs + i);
        }
    }
}

void gen_text_seg2(InterCodes* ics) {
    init_reg();
    println(".text");
    gen_read_func();
    gen_write_func();
    InterCodes* ic = ics;
    ic_start = ic_end = ics;
    while (ic_end != NULL) {
        peek_basic_block2(ics);
        for (ic = ic_start; ic != ic_end; ic = ic->next) {
            switch (ic->code.kind) {
                case IR_LABEL: {
                    println("label%d:", ic->code.result.u.label_id);
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_FUNC: {
                    println();
                    println("%s:", ic->code.result.symbol->name);
                    clear_lvas();
                    gen_prologue();
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_ASSIGN: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    printIns("move %s, %s", rr->name, r1->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_ADD: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    Reg* r2 = get_reg2(&ic->code.arg2);
                    printIns("add %s, %s, %s", rr->name, r1->name, r2->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, r2);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_SUB: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    Reg* r2 = get_reg2(&ic->code.arg2);
                    printIns("sub %s, %s, %s", rr->name, r1->name, r2->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, r2);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_MUL: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    Reg* r2 = get_reg2(&ic->code.arg2);
                    printIns("mul %s, %s, %s", rr->name, r1->name, r2->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, r2);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_DIV: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    Reg* r2 = get_reg2(&ic->code.arg2);
                    printIns("div %s, %s", r1->name, r2->name);
                    printIns("mflo %s", rr->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, r2);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_ADDR: {
                    Reg* rr = get_reg2(&ic->code.result);
                    gen_addr(rr, &ic->code.arg1);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_DEREF_L: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    printIns("sw %s, 0(%s)", r1->name, rr->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_DEREF_R: {
                    Reg* rr = get_reg2(&ic->code.result);
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    printIns("lw %s, 0(%s)", rr->name, r1->name);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, rr);
                    break;
                }
                case IR_GOTO: {
                    printIns("j label%d", ic->code.result.u.label_id);
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_RELOP: {
                    char *ins;
                    switch (ic->code.relop) {
                        case RELOP_EQ: ins = "beq"; break;
                        case RELOP_NE: ins = "bne"; break;
                        case RELOP_GT: ins = "bgt"; break;
                        case RELOP_LT: ins = "blt"; break;
                        case RELOP_GE: ins = "bge"; break;
                        case RELOP_LE: ins = "ble"; break;
                        default: assert(0);
                    }
                    Reg* r1 = get_reg2(&ic->code.arg1);
                    Reg* r2 = get_reg2(&ic->code.arg2);
                    printIns("%s %s, %s, label%d", ins, r1->name, r2->name,
                        ic->code.result.u.label_id);
                    try_free_reg(ic, r1);
                    try_free_reg(ic, r2);
                    break;
                }
                case IR_RETURN: {
                    Reg* rr = get_reg2(&ic->code.result);
                    printIns("move $v0, %s", rr->name);
                    // invoke gen_epilogue() after get_reg(...)
                    // avoid to reset $fp early
                    gen_epilogue();
                    printIns("jr $ra");
                    free_reg(rr);
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_DEC: {
                    add_array2lva(&ic->code.result, ic->code.size);
                    break;
                }
                case IR_ARG: {
                    // put all args on stack
                    Reg* rr = get_reg2(&ic->code.result);
                    printIns("addi $sp, $sp, -4");
                    printIns("sw %s, 0($sp)", rr->name);
                    free_reg(rr);
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_CALL: {
                    printIns("addi $sp, $sp, -4");
                    printIns("sw $ra, 0($sp)");
                    printIns("jal %s", ic->code.arg1.symbol->name);
                    printIns("lw $ra, 0($sp)");
                    printIns("addi $sp, $sp, 4");
                    // don't invoke get_reg(...) before 'jal ...'
                    // avoid to allocate stack between ARG... and CALL...
                    Reg* rr = get_reg2(&ic->code.result);
                    printIns("move %s, $v0", rr->name);
                    spill_reg(rr);
                    assert(ic_start + 1 == ic_end);
                    break;
                }
                case IR_PARAM: {
                    add_param2lva(&ic->code.result);
                    break;
                }
                case IR_READ: {
                    Reg* rr = get_reg2(&ic->code.result);
                    printIns("addi $sp, $sp, -4");
                    printIns("sw $ra, 0($sp)");
                    printIns("jal read");
                    printIns("lw $ra, 0($sp)");
                    printIns("addi $sp, $sp, 4");
                    printIns("move %s, $v0", rr->name);
                    spill_reg(rr);
                    break;
                }
                case IR_WRITE: {
                    Reg* rr = get_reg2(&ic->code.result);
                    printIns("move $a0, %s", rr->name);
                    free_reg(rr);

                    printIns("addi $sp, $sp, -4");
                    printIns("sw $ra, 0($sp)");
                    printIns("jal write");
                    printIns("lw $ra, 0($sp)");
                    printIns("addi $sp, $sp, 4");
                    break;
                }
                default: assert(0);
            }
        }
        spill_all_regs();
    }
}
