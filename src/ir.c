#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "AST.h"
#include "ir.h"
#include "sym_table.h"

InterCodes* newInterCodes() {
    InterCodes* p = (InterCodes*)malloc(sizeof(InterCodes));
    p->next = p->prev = NULL;
    return p;
}

ArgNode* newArgNode(int var_id) {
    ArgNode *arg = (ArgNode*)malloc(sizeof(ArgNode));
    arg->next = NULL;
    arg->var_id = var_id;
    return arg;
}

int newVariableId() {
    static int id = 1;
    return id++;
}

int newLabelId() {
    static int id = 1;
    return id++;
}

static InterCodes* getInterCodesTail(InterCodes* head) {
    if (head == NULL) return NULL;
    while (head->next != NULL) {
        head = head->next;
    }
    return head;
}

InterCodes* concatInterCodes(int count, ...) {
    // input can be:
    //     NULL code1 code2 NULL NULL code3 ...
    if (count < 1) return NULL;

    va_list argp;
    va_start(argp, count);

    InterCodes *head, *tail, *p;
    int i = 1;
    head = tail = va_arg(argp, InterCodes*);

    // find the first Not NULL value
    while (head == NULL && i < count) {
        head = tail = va_arg(argp, InterCodes*);
        i++;
    }
    if (i >= count) return head;

    assert(head);
    assert(tail);
    for (; i < count; i++) {
        p = va_arg(argp, InterCodes*);
        if (p == NULL)
            continue;
        tail = getInterCodesTail(tail);
        assert(tail);
        tail->next = p;
        p->prev = tail;
    }

    va_end(argp);
    return head;
}

InterCodes* genLabelCode(int label_id) {
    InterCodes* codes = newInterCodes();
    codes->code.kind = IR_LABEL;
    codes->code.result.kind = OP_LABEL;
    codes->code.result.u.label_id = label_id;
}

InterCodes* genGotoCode(int label_id) {
    InterCodes* codes = newInterCodes();
    codes->code.kind = IR_GOTO;
    codes->code.result.kind = OP_LABEL;
    codes->code.result.u.label_id = label_id;
}

enum RELOP_TYPE get_relop(ASTNode *RELOP) {
    assert(RELOP);
    assert(RELOP->type == AST_RELOP);

    if (strcmp(RELOP->val.c, "<") == 0) {
        return RELOP_LT;
    } else if (strcmp(RELOP->val.c, "<=") == 0) {
        return RELOP_LE;
    } else if (strcmp(RELOP->val.c, "==") == 0) {
        return RELOP_EQ;
    } else if (strcmp(RELOP->val.c, ">") == 0) {
        return RELOP_GT;
    } else if (strcmp(RELOP->val.c, ">=") == 0) {
        return RELOP_GE;
    } else if (strcmp(RELOP->val.c, "!=") == 0) {
        return RELOP_NE;
    } else {
        assert(0);
    }
}

InterCodes* translate_Exp(ASTNode* Exp, int place) {
    assert(Exp);
    assert(Exp->type == AST_Exp);

    InterCodes* codes = newInterCodes();
    if (Exp->child->type == AST_INT) { // Exp -> INT
        codes->code.kind = IR_ASSIGN;
        codes->code.result.kind = OP_TEMP;
        codes->code.result.u.var_id = place;
        codes->code.arg1.kind = OP_CONSTANT;
        codes->code.arg1.u.value = Exp->child->val.i;
    } else if (Exp->child->type == AST_ID && Exp->child->sibling == NULL) { // Exp -> ID
        codes->code.kind = IR_ASSIGN;
        codes->code.result.kind = OP_TEMP;
        codes->code.result.u.var_id = place;
        codes->code.arg1.kind = OP_VARIABLE;
        codes->code.arg1.symbol = lookupSymbol(Exp->child->val.c, true);
    } else if(Exp->child->type == AST_FLOAT) {
        assert(0);
    } else if (Exp->child->sibling->type == AST_ASSIGNOP) { // Exp -> EXP ASSIGNOP Exp
        if (Exp->child->child->type == AST_ID) { // Exp1 -> ID
            Symbol variable = lookupSymbol(Exp->child->child->val.c, true);
            int t1 = newVariableId();
            InterCodes* code1 = translate_Exp(Exp->child->sibling->sibling, t1);

            InterCodes* code2 = newInterCodes();
            code2->code.kind = IR_ASSIGN;
            code2->code.result.kind = OP_VARIABLE;
            code2->code.result.symbol = variable;
            code2->code.arg1.kind = OP_TEMP;
            code2->code.arg1.u.var_id = t1;

            InterCodes* code3 = newInterCodes();
            code3->code.kind = IR_ASSIGN;
            code3->code.result.kind = OP_TEMP;
            code3->code.result.u.var_id = place;
            code3->code.arg1.kind = OP_VARIABLE;
            code3->code.arg1.symbol = variable;

            codes = concatInterCodes(3, code1, code2, code3);
        }
        else {
            assert(0);
        }
    } else if (Exp->child->sibling->type == AST_PLUS) { // Exp -> EXP PLUS Exp
        int t1 = newVariableId();
        int t2 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);
        InterCodes* code2 = translate_Exp(Exp->child->sibling->sibling, t2);

        InterCodes* code3 = newInterCodes();
        code3->code.kind = IR_ADD;
        code3->code.result.kind = OP_TEMP;
        code3->code.result.u.var_id = place;
        code3->code.arg1.kind = OP_TEMP;
        code3->code.arg1.u.var_id = t1;
        code3->code.arg2.kind = OP_TEMP;
        code3->code.arg2.u.var_id = t2;

        codes = concatInterCodes(3, code1, code2, code3);
    } else if (Exp->child->sibling->type == AST_MINUS) { // Exp -> EXP MINUS Exp
        int t1 = newVariableId();
        int t2 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);
        InterCodes* code2 = translate_Exp(Exp->child->sibling->sibling, t2);

        InterCodes* code3 = newInterCodes();
        code3->code.kind = IR_SUB;
        code3->code.result.kind = OP_TEMP;
        code3->code.result.u.var_id = place;
        code3->code.arg1.kind = OP_TEMP;
        code3->code.arg1.u.var_id = t1;
        code3->code.arg2.kind = OP_TEMP;
        code3->code.arg2.u.var_id = t2;

        codes = concatInterCodes(3, code1, code2, code3);
    } else if (Exp->child->sibling->type == AST_STAR) { // Exp -> EXP STAR Exp
        int t1 = newVariableId();
        int t2 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);
        InterCodes* code2 = translate_Exp(Exp->child->sibling->sibling, t2);

        InterCodes* code3 = newInterCodes();
        code3->code.kind = IR_MUL;
        code3->code.result.kind = OP_TEMP;
        code3->code.result.u.var_id = place;
        code3->code.arg1.kind = OP_TEMP;
        code3->code.arg1.u.var_id = t1;
        code3->code.arg2.kind = OP_TEMP;
        code3->code.arg2.u.var_id = t2;

        codes = concatInterCodes(3, code1, code2, code3);
    } else if (Exp->child->sibling->type == AST_DIV) { // Exp -> EXP DIV Exp
        int t1 = newVariableId();
        int t2 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);
        InterCodes* code2 = translate_Exp(Exp->child->sibling->sibling, t2);

        InterCodes* code3 = newInterCodes();
        code3->code.kind = IR_DIV;
        code3->code.result.kind = OP_TEMP;
        code3->code.result.u.var_id = place;
        code3->code.arg1.kind = OP_TEMP;
        code3->code.arg1.u.var_id = t1;
        code3->code.arg2.kind = OP_TEMP;
        code3->code.arg2.u.var_id = t2;

        codes = concatInterCodes(3, code1, code2, code3);
    } else if (Exp->child->type == AST_MINUS) { // Exp -> MINUS Exp
        int t1 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);

        InterCodes* code2 = newInterCodes();
        code2->code.kind = IR_SUB;
        code2->code.result.kind = OP_TEMP;
        code2->code.result.u.var_id = place;
        code2->code.arg1.kind = OP_CONSTANT;
        code2->code.arg1.u.value = 0;
        code2->code.arg2.kind = OP_TEMP;
        code2->code.arg2.u.var_id = t1;

        codes = concatInterCodes(2, code1, code2);
    } else if (Exp->child->sibling->type == AST_RELOP ||
               Exp->child->sibling->type == AST_AND ||
               Exp->child->sibling->type == AST_OR ||
               Exp->child->type == AST_NOT) {
        int true_label = newLabelId();
        int false_label = newLabelId();

        InterCodes* code1 = newInterCodes();
        code1->code.kind = IR_ASSIGN;
        code1->code.result.kind = OP_TEMP;
        code1->code.result.u.var_id = place;
        code1->code.arg1.kind = OP_CONSTANT;
        code1->code.arg1.u.value = 0;

        InterCodes* code2 = translate_Cond(Exp, true_label, false_label);

        InterCodes* code3 = genLabelCode(true_label);

        InterCodes* code4 = newInterCodes();
        code4->code.kind = IR_ASSIGN;
        code4->code.result.kind = OP_TEMP;
        code4->code.result.u.var_id = place;
        code4->code.arg1.kind = OP_CONSTANT;
        code4->code.arg1.u.value = 1;

        InterCodes* code5 = genLabelCode(false_label);

        codes = concatInterCodes(5, code1, code2, code3, code4, code5);
    } else if (Exp->child->type == AST_ID && Exp->child->sibling->type == AST_LP) {
        if (Exp->child->sibling->sibling->type == AST_RP) { // ID LP RP
            Symbol func = lookupSymbol(Exp->child->val.c, true);
            if (strcmp(func->name, "read") == 0) {
                codes->code.kind = IR_READ;
                codes->code.result.kind = OP_TEMP;
                codes->code.result.u.var_id = place;
            } else {
                codes->code.kind = IR_CALL;
                codes->code.result.kind = OP_TEMP;
                codes->code.result.u.var_id = place;
                codes->code.arg1.kind = OP_FUNCTION;
                codes->code.arg1.symbol = func;
            }
        } else { // ID LP Args RP
            Symbol func = lookupSymbol(Exp->child->val.c, true);

            ArgNode* arg_list = NULL;
            InterCodes* code1 = translate_Args(Exp->child->sibling->sibling, &arg_list);
            if (strcmp(func->name, "write") == 0) {
                assert(arg_list);
                assert(arg_list->next == NULL);
                InterCodes* code2 = newInterCodes();
                code2->code.kind = IR_WRITE;
                code2->code.result.kind = OP_TEMP;
                code2->code.result.u.var_id = arg_list->var_id;
                codes = concatInterCodes(2, code1, code2);
            } else {
                InterCodes* code2 = NULL;
                for (ArgNode* p = arg_list; p != NULL; p = p->next) {
                    InterCodes* tmp_code = newInterCodes();
                    tmp_code->code.kind = IR_ARG;
                    tmp_code->code.result.kind = OP_TEMP;
                    tmp_code->code.result.u.var_id = arg_list->var_id;
                    if (code2 == NULL) {
                        code2 = tmp_code;
                    } else {
                        code2 = concatInterCodes(2, code2, tmp_code);
                    }
                }

                InterCodes* code3 = newInterCodes();
                code3->code.kind = IR_CALL;
                code3->code.result.kind = OP_TEMP;
                code3->code.result.u.var_id = place;
                code3->code.arg1.kind = OP_FUNCTION;
                code3->code.arg1.symbol = func;

                codes = concatInterCodes(3, code1, code2, code3);
                // TODO: dealloc arg_list
            }
        }
    }
    else {
        assert(0);
    }

    return codes;
}

InterCodes* translate_Stmt(ASTNode *Stmt) {
    assert(Stmt);
    assert(Stmt->type == AST_Stmt);

    InterCodes* codes = NULL;

    if (Stmt->child->type == AST_CompSt) { // Stmt -> CompSt
        codes = translate_CompSt(Stmt->child);
    } else if (Stmt->child->type == AST_Exp) { // Stmt -> Exp SEMI
        codes = translate_Exp(Stmt->child, VAR_NULL);
    } else if (Stmt->child->type == AST_RETURN) { // Stmt -> RETURN Exp SEMI
        int t1 = newVariableId();
        InterCodes* code1 = translate_Exp(Stmt->child->sibling, t1);

        InterCodes* code2 = newInterCodes();
        code2->code.kind = IR_RETURN;
        code2->code.result.kind = OP_TEMP;
        code2->code.result.u.var_id = t1;

        codes = concatInterCodes(2, code1, code2);
    } else if (Stmt->child->type == AST_WHILE) { // Stmt -> WHILE LP Exp RP Stmt
        int label1 = newLabelId();
        int label2 = newLabelId();
        int label3 = newLabelId();

        InterCodes* code1 = translate_Cond(Stmt->child->sibling->sibling, label2, label3);
        InterCodes* code2 = translate_Stmt(Stmt->child->sibling->sibling->sibling->sibling);

        codes = concatInterCodes(6, genLabelCode(label1), code1, genLabelCode(label2),
                                    code2, genGotoCode(label1), genLabelCode(label3));
    } else if (Stmt->child->type == AST_IF) {
        if (Stmt->child->sibling->sibling->sibling->sibling->sibling == NULL) { // Stmt -> IF LP Exp RP Stmt
            int label1 = newLabelId();
            int label2 = newLabelId();
            InterCodes* code1 = translate_Cond(Stmt->child->sibling->sibling, label1, label2);
            InterCodes* code2 = translate_Stmt(Stmt->child->sibling->sibling->sibling->sibling);
            codes = concatInterCodes(4, code1, genLabelCode(label1), code2, genLabelCode(label2));
        } else { // IF LP Exp RP Stmt ELSE Stmt
            int label1 = newLabelId();
            int label2 = newLabelId();
            int label3 = newLabelId();
            InterCodes* code1 = translate_Cond(Stmt->child->sibling->sibling, label1, label2);
            InterCodes* code2 = translate_Stmt(Stmt->child->sibling->sibling->sibling->sibling);
            InterCodes* code3 = translate_Stmt(Stmt->child->sibling->sibling->sibling->sibling->sibling->sibling);
            codes = concatInterCodes(7, code1, genLabelCode(label1), code2,
                                        genGotoCode(label3), genLabelCode(label2),
                                        code3, genLabelCode(label3));
        }
    } else {
        assert(0);
    }

    return codes;
}

InterCodes* translate_StmtList(ASTNode *StmtList) {
    assert(StmtList);
    assert(StmtList->type == AST_StmtList);

    InterCodes* codes = NULL;

    if (StmtList->child != NULL) {
        codes = translate_Stmt(StmtList->child);
        InterCodes* code2 = translate_StmtList(StmtList->child->sibling);
        if (code2 != NULL) {
            codes = concatInterCodes(2, codes, code2);
        }
    }

    return codes;
}

InterCodes* translate_CompSt(ASTNode *CompSt) {
    assert(CompSt);
    assert(CompSt->type == AST_CompSt);

    InterCodes* code1 = translate_DefList(CompSt->child->sibling);
    InterCodes* code2 = translate_StmtList(CompSt->child->sibling->sibling);

    return concatInterCodes(2, code1, code2);
}

InterCodes* translate_DefList(ASTNode *DefList) {
    assert(DefList);
    assert(DefList->type == AST_DefList);

    InterCodes* codes = NULL;

    if (DefList->child != NULL) {
        codes = translate_Def(DefList->child);
        InterCodes* code2 = translate_DefList(DefList->child->sibling);
        if (code2 != NULL) {
            codes = concatInterCodes(2, codes, code2);
        }
    }

    return codes;
}

InterCodes* translate_Def(ASTNode *Def) {
    assert(Def);
    assert(Def->type == AST_Def);

    return translate_DecList(Def->child->sibling);
}

InterCodes* translate_DecList(ASTNode *DecList) {
    assert(DecList);
    assert(DecList->type == AST_DecList);

    InterCodes* codes = NULL;

    codes = translate_Dec(DecList->child);

    if (DecList->child->sibling != NULL) {
        InterCodes* code2 = translate_DecList(DecList->child->sibling->sibling);
        if (code2 != NULL) {
            codes = concatInterCodes(2, codes, code2);
        }
    }

    return codes;
}

InterCodes* translate_Dec(ASTNode *Dec) {
    assert(Dec);
    assert(Dec->type == AST_Dec);

    InterCodes* codes;

    if (Dec->child->child->sibling != NULL) {
        assert(0);
    }

    if (Dec->child->sibling != NULL) { // Dec -> VarDec ASSIGNOP Exp
        int t1 = newVariableId();
        Symbol variable = lookupSymbol(Dec->child->child->val.c, true);
        InterCodes* code1 = translate_Exp(Dec->child->sibling->sibling, t1);

        InterCodes* code2 = newInterCodes();
        code2->code.kind = IR_ASSIGN;
        code2->code.result.kind = OP_VARIABLE;
        code2->code.result.symbol = variable;
        code2->code.arg1.kind = OP_TEMP;
        code2->code.arg1.u.var_id = t1;

        codes = concatInterCodes(2, code1, code2);

    }

    return codes;
}

InterCodes* translate_Cond(ASTNode *Exp, int label_true, int label_false) {
    assert(Exp);
    assert(Exp->type == AST_Exp);

    InterCodes* codes = NULL;
    if (Exp->child->type == AST_NOT) { // Exp -> NOT Exp
        codes = translate_Cond(Exp, label_false, label_true);
    } else if (Exp->child->sibling->type == AST_RELOP) { // Exp -> Exp RELOP Exp
        int t1 = newVariableId();
        int t2 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp->child, t1);
        InterCodes* code2 = translate_Exp(Exp->child->sibling->sibling, t2);

        InterCodes* code3 = newInterCodes();
        code3->code.kind = IR_RELOP;
        code3->code.relop = get_relop(Exp->child->sibling);
        code3->code.result.kind = OP_LABEL;
        code3->code.result.u.label_id = label_true;
        code3->code.arg1.kind = OP_TEMP;
        code3->code.arg1.u.var_id = t1;
        code3->code.arg2.kind = OP_TEMP;
        code3->code.arg2.u.var_id = t2;

        codes = concatInterCodes(4, code1, code2, code3, genGotoCode(label_false));
    } else if (Exp->child->sibling->type == AST_AND) { // Exp AND Exp
        int label1 = newLabelId();
        InterCodes* code1 = translate_Cond(Exp->child, label1, label_false);
        InterCodes* code2 = translate_Cond(Exp->child->sibling->sibling, label_true, label_false);
        codes = concatInterCodes(3, code1, genLabelCode(label1), code2);
    } else if (Exp->child->sibling->type == AST_OR) { // Exp OR Exp
        int label1 = newLabelId();
        InterCodes* code1 = translate_Cond(Exp->child, label_true, label1);
        InterCodes* code2 = translate_Cond(Exp->child->sibling->sibling, label_true, label_false);
        codes = concatInterCodes(3, code1, genLabelCode(label1), code2);
    } else {
        int t1 = newVariableId();
        InterCodes* code1 = translate_Exp(Exp, t1);

        InterCodes* code2 = newInterCodes();
        code2->code.kind = IR_RELOP;
        code2->code.relop = RELOP_NE;
        code2->code.result.kind = OP_LABEL;
        code2->code.result.u.label_id = label_true;
        code2->code.arg1.kind = OP_TEMP;
        code2->code.arg1.u.var_id = t1;
        code2->code.arg2.kind = OP_CONSTANT;
        code2->code.arg2.u.var_id = 0;

        codes = concatInterCodes(3, code1, code2, genGotoCode(label_false));
    }
    assert(codes);
    return codes;
}

InterCodes* translate_Args(ASTNode *Args, ArgNode** arg_list) {
    assert(Args);
    assert(Args->type == AST_Args);

    int t1 = newVariableId();
    InterCodes* codes = translate_Exp(Args->child, t1);
    ArgNode* arg = newArgNode(t1);
    // append arg before arg_list
    if (*arg_list == NULL) {
        *arg_list = arg;
    } else {
        ArgNode* p = *arg_list;
        *arg_list = arg;
        (*arg_list)->next = p;
    }

    if (Args->child->sibling == NULL) { // Args -> Exp
        return codes;
    } else { // Args -> Exp COMMA Args
        InterCodes* code2 = translate_Args(Args->child->sibling->sibling, arg_list);
        return concatInterCodes(2, codes, code2);
    }
}

InterCodes* translate_Program(ASTNode *Program) {
    assert(Program);
    assert(Program->type == AST_Program);
    return translate_ExtDefList(Program->child);
}

InterCodes* translate_ExtDefList(ASTNode *ExtDefList) {
    assert(ExtDefList);
    assert(ExtDefList->type == AST_ExtDefList);

    InterCodes* codes = NULL;

    if (ExtDefList->child != NULL) {
        codes = translate_ExtDef(ExtDefList->child);
        InterCodes* code2 = translate_ExtDefList(ExtDefList->child->sibling);
        if (code2 != NULL) {
            codes = concatInterCodes(2, codes, code2);
        }
    }

    return codes;
}

InterCodes* translate_ExtDef(ASTNode *ExtDef) {
    assert(ExtDef);
    assert(ExtDef->type == AST_ExtDef);

    InterCodes* codes = NULL;

    if (ExtDef->child->sibling->type == AST_FunDec) {
        codes = translate_FunDec(ExtDef->child->sibling);
        if (ExtDef->child->sibling->sibling->type == AST_CompSt) {
            InterCodes* code = translate_CompSt(ExtDef->child->sibling->sibling);
            codes = concatInterCodes(2, codes, code);
        } else {
            assert(0);
        }
    }

    return codes;
}

InterCodes* translate_FunDec(ASTNode *FunDec) {
    assert(FunDec);
    assert(FunDec->type == AST_FunDec);

    InterCodes* codes = newInterCodes();
    codes->code.kind = IR_FUNC;
    codes->code.result.kind = OP_FUNCTION;
    codes->code.result.symbol = lookupSymbol(FunDec->child->val.c, true);

    if (FunDec->child->sibling->sibling->type == AST_VarList) {
        codes = concatInterCodes(2, codes, translate_VarList(FunDec->child->sibling->sibling));
    }

    return codes;
}

InterCodes* translate_VarList(ASTNode *VarList) {
    assert(VarList);
    assert(VarList->type == AST_VarList);

    InterCodes* codes = translate_ParamDec(VarList->child);

    if (VarList->child->sibling != NULL) {
        codes = concatInterCodes(2, codes, translate_VarList(VarList->child->sibling->sibling));
    }

    return codes;
}

InterCodes* translate_ParamDec(ASTNode *ParamDec) {
    assert(ParamDec);
    assert(ParamDec->type == AST_ParamDec);

    Symbol variable = lookupSymbol(ParamDec->child->sibling->child->val.c, true);

    InterCodes* codes = newInterCodes();
    codes->code.kind = IR_PARAM;
    codes->code.result.kind = OP_VARIABLE;
    codes->code.result.symbol = variable;

    return codes;
}

static void printOperand(Operand op) {
    if (op.kind == OP_TEMP) {
        printf("t%d", op.u.var_id);
    } else if (op.kind == OP_VARIABLE || op.kind == OP_FUNCTION) {
        printf("%s", op.symbol->name);
    } else if (op.kind == OP_CONSTANT) {
        printf("#%d", op.u.value);
    } else if (op.kind == OP_LABEL) {
        printf("label%d", op.u.label_id);
    } else {
        assert(0);
    }
}

void generate_ir(ASTNode* Program) {
    InterCodes* codes = translate_Program(Program);

    for (InterCodes* p = codes; p != NULL; p = p->next) {
        switch(p->code.kind) {
            case IR_LABEL: {
                assert(p->code.result.kind == OP_LABEL);
                printf("LABEL label%d :\n", p->code.result.u.label_id);
                break;
            }
            case IR_FUNC: {
                assert(p->code.result.kind == OP_FUNCTION);
                printf("FUNCTION %s :\n", p->code.result.symbol->name);
                break;
            }
            case IR_ASSIGN: {
                if (p->code.result.kind == OP_TEMP && p->code.result.u.var_id == VAR_NULL) {
                    break;
                }
                printOperand(p->code.result);
                printf(" := ");
                printOperand(p->code.arg1);
                printf("\n");
                break;
            }
            case IR_ADD: {
                printOperand(p->code.result);
                printf(" := ");
                printOperand(p->code.arg1);
                printf(" + ");
                printOperand(p->code.arg2);
                printf("\n");
                break;
            }
            case IR_SUB: {
                printOperand(p->code.result);
                printf(" := ");
                printOperand(p->code.arg1);
                printf(" - ");
                printOperand(p->code.arg2);
                printf("\n");
                break;
            }
            case IR_MUL: {
                printOperand(p->code.result);
                printf(" := ");
                printOperand(p->code.arg1);
                printf(" * ");
                printOperand(p->code.arg2);
                printf("\n");
                break;
            }
            case IR_DIV: {
                printOperand(p->code.result);
                printf(" := ");
                printOperand(p->code.arg1);
                printf(" / ");
                printOperand(p->code.arg2);
                printf("\n");
                break;
            }
            case IR_GOTO: {
                printf("GOTO ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_RELOP: {
                printf("IF ");
                printOperand(p->code.arg1);
                switch (p->code.relop) {
                    case RELOP_LT: printf(" < "); break;
                    case RELOP_LE: printf(" <= "); break;
                    case RELOP_EQ: printf(" == "); break;
                    case RELOP_GT: printf(" > "); break;
                    case RELOP_GE: printf(" >= "); break;
                    case RELOP_NE: printf(" != "); break;
                    default: assert(0);
                }
                printOperand(p->code.arg2);
                printf(" GOTO ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_RETURN: {
                printf("RETURN ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_ARG: {
                printf("ARG ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_CALL: {
                printOperand(p->code.result);
                printf(" := CALL ");
                printOperand(p->code.arg1);
                printf("\n");
                break;
            }
            case IR_PARAM: {
                printf("PARAM ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_READ: {
                printf("READ ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            case IR_WRITE: {
                printf("WRITE ");
                printOperand(p->code.result);
                printf("\n");
                break;
            }
            default: assert(0);
        }
    }
}
