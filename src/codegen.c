#include "rvcc.h"

// reg byte width
const int REG_BYTES = 4;

// Record stack depth
static int Depth = 0;

static int align_to(int N, int align) {
    return (N + align - 1) / align * align;
}

static void push() {
    printf("    # a0 is pressed into a stack\n");
    printf("    addi sp, sp, %d\n", -REG_BYTES);
    printf("    sw a0, 0(sp)\n");
    ++Depth;
}

static void pop(const char *reg) {
    printf("    lw %s, 0(sp)\n", reg);
    printf("    addi sp, sp, %d\n", REG_BYTES);
    --Depth;
}

static void gen_var_addr(Node *nd) {
    if (nd->kind == ND_VAR) {
        printf("    # Get the variable stack address and store a0\n");
        printf("    addi a0, fp, %d\n", nd->var->offset);
        return;
    }
    error("not an lvalue");
}

static int count_code_segment() {
    static int i = 1;
    return i++;
}

static void gen_expr(Node *nd) {
    // if ast have only one num node
    switch (nd->kind) {
        case ND_NUM:
            printf("    # Assign a value to ao\n");
            printf("    li a0, %d\n", nd->val);
            return;
        case ND_NEG:
            gen_expr(nd->lhs);
            printf("    # Take the negative of ao\n");
            printf("    neg a0, a0\n");
            return;
        case ND_ASSIGN:
            gen_var_addr(nd->lhs);
            push();
            gen_expr(nd->rhs);
            printf("    # pop stack to a1\n");
            pop("a1");
            printf("    # store ao to 0(a1)\n");
            printf("    sw a0, 0(a1)\n");
            return;
        case ND_VAR:
            gen_var_addr(nd);
            printf("    # load onto a0 from 0(a0)\n");
            printf("    lw a0, 0(a0)\n");
            return;
    }

    // Recurse to the rightmost node
    gen_expr(nd->rhs);
    // Press the a0 register value onto the stack
    push();
    // Recurse to the leftmost node
    gen_expr(nd->lhs);
    // Pop the right subtree value onto the stack
    printf("    # pop stack to a1\n");
    pop("a1");

    // Judgment operation
    switch (nd->kind) {
        case ND_ADD:
            printf("    # a0 = a0 + a1\n");
            printf("    add a0, a0, a1\n");  // a0 = a0 + a1
            break;
        case ND_SUB:
            printf("    # a0 = a0 - a1\n");
            printf("    sub a0, a0, a1\n");  // a0 = a0 - a1
            break;
        case ND_MUL:
            printf("    # a0 = a0 * a1\n");
            printf("    mul a0, a0, a1\n");  // a0 = a0 * a1
            break;
        case ND_DIV:
            printf("    # a0 = a0 / a1\n");
            printf("    div a0, a0, a1\n");  // a0 = a0 / a1
            break;
        case ND_EQ:
        case ND_NE:
            printf("    # a0 = a0 ^ a1\n");
            printf("    xor a0, a0, a1\n");  // a0 = a0 ^ a1
            if (nd->kind == ND_EQ) {
                printf("    # if a0 == 0, set a0 = 1\n");
                printf("    seqz a0, a0\n");  // if a0 == 0, set a0 = 1
            } else {
                printf("    # if a0 != 0, set a0 = 1\n");
                printf("    snez a0, a0\n");  // if a0 != 0, set a0 = 1
            }
            break;
        case ND_LE:
            printf("    # if a1 < a0, set a0 = 1\n");
            printf("    slt a0, a1, a0\n");
            printf("    # a0 = a0 ^ 1\n");
            printf("    xori a0, a0, 1\n");
            break;
        case ND_LT:
            printf("    # if a0 < a1, set a0 = 1\n");
            printf("    slt a0, a0, a1\n");
            break;
        default:
            error("invalid expr");
            break;
    }
}

static void gen_stmt(Node *nd) {
    switch (nd->kind) {
        case ND_FOR: {
            int i = count_code_segment();
            if (nd->init) {
                gen_stmt(nd->init);
            }
            printf(".L.begin.%d:\n", i);
            if (nd->cond) {
                gen_expr(nd->cond);
                printf("    # if a0 == 0, jump to .L.end.%d\n", i);
                printf("    beqz a0, .L.end.%d\n", i);
            }
            gen_stmt(nd->then);
            if (nd->inc) {
                gen_expr(nd->inc);
            }
            printf("    j .L.begin.%d\n", i);
            printf(".L.end.%d:\n", i);
            return;
        }
        case ND_IF: {
            int i = count_code_segment();
            gen_expr(nd->cond);
            printf("    # if a0 == 0, jump to .L.else.%d\n", i);
            printf("    beqz a0, .L.else.%d\n", i);
            gen_stmt(nd->then);
            printf("    j .L.end.%d\n", i);
            printf(".L.else.%d:\n", i);
            if (nd->els) {
                gen_stmt(nd->els);
            }
            printf(".L.end.%d:\n", i);
            return;
        }
        case ND_BLOCK:
            for (Node *n = nd->body; n != NULL; n = n->next) {
                gen_stmt(n);
            }
            return;
        case ND_EXPR_STMT:
            gen_expr(nd->lhs);
            return;
        case ND_RETURN:
            gen_expr(nd->lhs);
            printf("    j .L.return\n");
            return;
        default:
            error("invalid stmt");
    }
}

void calculate_var_offset(Function *prog) {
    int offset = 0;
    for (Object *var = prog->locals; var != NULL; var = var->next) {
        offset += REG_BYTES;
        var->offset = -offset;
    }
    prog->stack_size = align_to(offset, 16);
}

void codegen(Function *prog) {
    // calculate var stack offset
    calculate_var_offset(prog);
    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");

    printf("    # press fp onto the stack\n");
    printf("    addi sp, sp, %d\n", -REG_BYTES);
    printf("    sw fp, 0(sp)\n");
    printf("    # Assign the sp address to fp\n");
    printf("    mv fp, sp\n");

    printf("    # Allocate space on the stack for variables, algin to 16 Byte\n");
    printf("    addi sp, sp, %d\n", -prog->stack_size);

    // code generate
    gen_stmt(prog->body);
    assert(Depth == 0);

    printf(".L.return:\n");

    printf("    # Release a variable on the stack\n");
    printf("    mv sp, fp\n");
    printf("    # pop stack onto fp\n");
    printf("    lw fp, 0(sp)\n");
    printf("    addi sp, sp, %d\n", REG_BYTES);

    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");
}