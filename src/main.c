#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

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

    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");
    // li is the addi alias instruction, Load an immediate count to the register
    // parse (num)
    printf("    li a0, %d\n", get_number(tok));
    tok = tok->next;

    // parse (op num)
    while (tok->kind != TK_EOF)
    {
        if (str_equal(tok, "+"))
        {
            tok = tok->next;
            printf("    addi a0, a0, %d\n", get_number(tok));
            tok = tok->next;
        }
        else if (str_equal(tok, "-"))
        {
            tok = tok->next;
            printf("    addi a0, a0, -%d\n", get_number(tok));
            tok = tok->next;
        }
        else
        {
            error_tok(tok, "expected a operator");
        }
    }

    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");
}