#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

char *current_input;

typedef enum
{
    TK_PUNCT,
    TK_NUM,
    TK_EOF
} TokenKind;

typedef struct Token Token;
struct Token
{
    TokenKind kind;
    int val;
    Token *next;
    const char *loc;
    int len;
};

static Token *new_token(TokenKind kind, const char *start, const char *end)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

static void error(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char str[1024] = {};
    vsprintf(str, fmt, va);
    fprintf(stderr, "%s\n", str);
    va_end(va);
    exit(1);
}

static void verror_at(const char *loc, const char *fmt, va_list va)
{
    // output source code info
    fprintf(stderr, "%s\n", current_input);
    // calculate error location
    int len = loc - current_input;
    fprintf(stderr, "%*s", len, "");
    fprintf(stderr, "^ ");
    char str[1024] = {};
    vsprintf(str, fmt, va);
    fprintf(stderr, "%s\n", str);
    va_end(va);
    exit(1);
}

static void error_at(const char *loc, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    verror_at(loc, fmt, va);
}

static void error_tok(const Token *tok, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    verror_at(tok->loc, fmt, va);
}

static int get_number(Token *tok)
{
    if (tok->kind != TK_NUM)
    {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

static bool str_equal(Token *tok, const char *str)
{
    return memcmp(tok->loc, str, tok->len) == 0 && str[tok->len] == '\0';
}

static Token *tokenize(char *p)
{
    Token head = {};
    Token *cur = &head;
    while (*p)
    {
        if (isspace(*p))
        {
            p++;
        }
        else if (isdigit(*p))
        {
            cur->next = new_token(TK_NUM, p, p);
            cur = cur->next;
            const char *old_p = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - old_p;
        }
        else if (ispunct(*p))
        {
            cur->next = new_token(TK_PUNCT, p, p + 1);
            cur = cur->next;
            p++;
        }
        else
        {
            error_at(p, "invalid token\n");
        }
    }

    cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

typedef enum
{
    ND_NUM,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV
} NodeKind;

typedef struct Node Node;
struct Node
{
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
};

static Node *new_node(NodeKind kind)
{
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->lhs = NULL;
    nd->rhs = NULL;
    return nd;
}

static Node *new_num(int val)
{
    Node *nd = new_node(ND_NUM);
    nd->val = val;
    return nd;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *nd = new_node(kind);
    nd->lhs = lhs;
    nd->rhs = rhs;
    return nd;
}

static Token *skip(Token *tok, const char *str)
{
    if (!str_equal(tok, str))
    {
        error_tok(tok, "expected %s", str);
    }
    return tok->next;
}

static Node *expr(Token **rest, Token *tok);

// primary = (expr) | num
static Node *primary(Token **rest, Token *tok)
{
    // (expr)
    if (str_equal(tok, "("))
    {
        Node *nd = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return nd;
    }
    else if (tok->kind == TK_NUM)
    {
        Node *nd = new_num(tok->val);
        *rest = tok->next;
        return nd;
    }
    else
    {
        error_tok(tok, "expected a expression or a num");
    }
}

// mul = primary (*primary | /primary)
static Node *mul(Token **rest, Token *tok)
{
    // parse primary
    Node *nd = primary(&tok, tok);
    // parse binary operator
    while (true)
    {
        if (str_equal(tok, "*"))
        {
            nd = new_binary(ND_MUL, nd, primary(&tok, tok->next));
        }
        else if (str_equal(tok, "/"))
        {
            nd = new_binary(ND_DIV, nd, primary(&tok, tok->next));
        }
        else
        {
            *rest = tok;
            return nd;
        }
    }
}

// expr = mul (+mul | -mul)
static Node *expr(Token **rest, Token *tok)
{
    // parse mul
    Node *nd = mul(&tok, tok);

    // +mul | -mul
    while (true)
    {
        if (str_equal(tok, "+"))
        {
            nd = new_binary(ND_ADD, nd, mul(&tok, tok->next));
        }
        else if (str_equal(tok, "-"))
        {
            nd = new_binary(ND_SUB, nd, mul(&tok, tok->next));
        }
        else
        {
            break;
        }
    }

    *rest = tok;
    return nd;
}

// Record stack depth
static int Depth = 0;

static void push()
{
    printf("    addi sp, sp, -4\n");
    printf("    sw a0, 0(sp)\n");
    ++Depth;
}

static void pop(const char *reg)
{
    printf("    lw %s, 0(sp)\n", reg);
    printf("    addi sp, sp, 4\n");
    --Depth;
}

static void gen_expr(Node *nd)
{
    // if ast have only one num node
    if (nd->kind == ND_NUM)
    {
        printf("    li a0, %d\n", nd->val);
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
    switch (nd->kind)
    {
    case ND_ADD:
        printf("    add a0, a0, a1\n"); // a0 = a0 + a1
        break;
    case ND_SUB:
        printf("    sub a0, a0, a1\n"); // a0 = a0 - a1
        break;
    case ND_MUL:
        printf("    mul a0, a0, a1\n"); // a0 = a0 * a1
        break;
    case ND_DIV:
        printf("    div a0, a0, a1\n"); // a0 = a0 / a1
        break;
    default:
        error("invalid expression");
        break;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s: invalid number of arguments!", argv[1]);
        return -1;
    }

    current_input = argv[1];
    // Lexical analysis, generate token
    Token *tok = tokenize(current_input);
    // build ast
    Node *nd = expr(&tok, tok);
    if (tok->kind != TK_EOF)
    {
        error_tok(tok, "extra token");
    }

    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");

    // code generate
    gen_expr(nd);

    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");
    
    assert(Depth == 0);
    return 0;
}