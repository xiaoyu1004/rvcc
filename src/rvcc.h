#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TK_PUNCT, TK_NUM, TK_EOF } TokenKind;

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
    ND_EXPR_STMT
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *next;
    Node *lhs;
    Node *rhs;
    int val;
};
Node *parse(Token *tok);

void codegen(Node *nd);