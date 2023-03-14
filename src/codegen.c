#include "rvcc.h"

// Record stack depth
static int Depth = 0;

static void push() {
    printf("    addi sp, sp, -4\n");
    printf("    sw a0, 0(sp)\n");
    ++Depth;
}

static void pop(const char *reg) {
    printf("    lw %s, 0(sp)\n", reg);
    printf("    addi sp, sp, 4\n");
    --Depth;
}

static void gen_expr(Node *nd) {
    // if ast have only one num node
    if (nd->kind == ND_NUM) {
        printf("    li a0, %d\n", nd->val);
        return;
    }

    if (nd->kind == ND_NEG) {
        gen_expr(nd->lhs);
        printf("    neg a0, a0\n");
        return;
    }

    // Recurse to the rightmost node
    gen_expr(nd->rhs);
    // Press the a0 register value onto the stack
    push();
    // Recurse to the leftmost node
    gen_expr(nd->lhs);
    // Pop the right subtree value onto the stack
    pop("a1");

    // Judgment operation
    switch (nd->kind) {
        case ND_ADD:
            printf("    add a0, a0, a1\n");  // a0 = a0 + a1
            break;
        case ND_SUB:
            printf("    sub a0, a0, a1\n");  // a0 = a0 - a1
            break;
        case ND_MUL:
            printf("    mul a0, a0, a1\n");  // a0 = a0 * a1
            break;
        case ND_DIV:
            printf("    div a0, a0, a1\n");  // a0 = a0 / a1
            break;
        case ND_EQ:
        case ND_NE:
            printf("    xor a0, a0, a1\n");  // a0 = a0 ^ a1
            if (nd->kind == ND_EQ) {
                printf("    seqz a0, a0\n");  // if a0 == 0, set a0 = 1
            } else {
                printf("    snez a0, a0\n");  // if a0 != 0, set a0 = 1
            }
            break;
        case ND_LE:
            printf("    slt a0, a1, a0\n");
            printf("    xori a0, a0, 1\n");
            break;
        case ND_LT:
            printf("    slt a0, a0, a1\n");
            break;
        default:
            error("invalid expression");
            break;
    }
}

static void gen_stmt(Node *nd) {
    if (nd->kind != ND_EXPR_STMT) {
        error("invalid expression");
    }

    gen_expr(nd->lhs);
}

void codegen(Node *nd) {
    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");

    // code generate
    for (Node *node = nd; node != NULL; node = node->next) {
        gen_stmt(node);
        assert(Depth == 0);
    }

    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");

    
}