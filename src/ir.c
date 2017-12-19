#include "ir.h"
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "AST.h"

InterCodes* newInterCodes() {
    InterCodes* p = (InterCodes*)malloc(sizeof(InterCodes));
    p->next = p->prev = NULL;
    return p;
}

static InterCodes* getInterCodesTail(InterCodes* head) {
    if (head == NULL) return NULL;
    while (head->next != NULL) {
        head = head->next;
    }
    return head;
}

InterCodes* concatInterCodes(int count, ...) {
    if (count < 1) return NULL;

    va_list argp;
    va_start(argp, count);

    InterCodes *head, *tail, *p;
    head = tail = va_arg(argp, InterCodes*);

    for (int i = 1; i < count; i++) {
        p = va_arg(argp, InterCodes*);
        tail = getInterCodesTail(tail);
        tail->next = p;
        p->prev = tail;
    }

    va_end(argp);
    return head;
}

InterCodes* translate_Exp(ASTNode* exp) {
    // assert(exp);
    // assert(exp->type == AST_Exp);

    // InterCodes* codes = newInterCodes();
    // if(exp->child->type == AST_INT){
    //     codes->code.kind = IR_ASSIGN;
    //     codes->code.u.assign.left.kind = VARIABLE;
    //     codes->code.u.assign.left.u.var_no = 0;
    // }
    return NULL;
}