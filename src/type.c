#include "rvcc.h"

Type *TypeInt = &(Type){TY_INT};

bool is_int(Type *type) {
    return type->kind == TY_INT;
}

bool is_ptr(Type *type) {
    return type->kind == TY_PTR && type->base != NULL;
}

Type *pointer_to(Type *base) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_PTR;
    type->base = base;
    return type;
}

Type *func_type(Type *ret_type) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = TY_FUNC;
    type->name = ret_type->name;
    type->ret_type = ret_type;
    return type;
}

Type *copy_type(Type *type) {
    Type *ty = calloc(1, sizeof(Type));
    *ty = *type;
    return ty;
}

void add_type(Node *nd) {
    if (nd == NULL || nd->type != NULL) {
        return;
    }

    add_type(nd->lhs);
    add_type(nd->rhs);
    add_type(nd->init);
    add_type(nd->cond);
    add_type(nd->inc);
    add_type(nd->then);
    add_type(nd->els);

    for (Node *n = nd->body; n != NULL; n = n->next) {
        add_type(n);
    }

    switch (nd->kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_NEG:
        case ND_ASSIGN:
            nd->type = nd->lhs->type;
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_NUM:
        case ND_FUNCCALL:
            nd->type = TypeInt;
            return;
        case ND_VAR:
            nd->type = nd->var->type;
            return;
        case ND_ADDR:
            nd->type = pointer_to(nd->lhs->type);
            return;
        case ND_DEREF:
            if (nd->lhs->type->kind != TY_PTR) {
                error_tok(nd->tok, "invalid pointer dereference");
            }
            nd->type = nd->lhs->type->base;
            return;
        default:
            break;
    }
}