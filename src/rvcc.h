#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TK_IDENT, TK_PUNCT, TK_NUM, TK_KEYWORD, TK_EOF } TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    int val;
    Token *next;
    const char *loc;
    int len;
};

void error(char *fmt, ...);
void error_at(const char *loc, const char *fmt, ...);
void verror_at(const char *loc, const char *fmt, va_list va);
void error_tok(const Token *tok, const char *fmt, ...);

bool str_equal(Token *tok, const char *str);

Token *tokenize(char *p);

typedef enum {
    ND_NUM,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_EXPR_STMT,
    ND_ASSIGN,
    ND_VAR,
    ND_RETURN,
    ND_IF,
    ND_BLOCK
} NodeKind;

typedef struct Node Node;

typedef struct Object Object;
struct Object {
    Object *next;
    char *name;
    int offset;
};

typedef struct Function Function;
struct Function {
    Node *body;
    Object *locals;
    int stack_size;
};

struct Node {
    NodeKind kind;
    Node *next;
    Node *lhs;
    Node *rhs;
    int val;
    Object *var;
    Node *body;
    Node *cond;
    Node *then;
    Node *els;
};

Function *parse(Token *tok);

void codegen(Function *nd);