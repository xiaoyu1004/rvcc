#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// reg byte width
#define REG_BYTES 4

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
bool consume(Token **rest, Token *tok, const char *str);

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
    ND_FOR,
    ND_BLOCK,
    ND_ADDR,
    ND_DEREF,
    ND_FUNCCALL
} NodeKind;

typedef struct Type Type;
typedef struct Node Node;

typedef struct Object Object;
struct Object {
    Object *next;
    Type *type;
    const char *name;
    int offset;
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Node *body;
    Object *params;
    Object *locals;
    int stack_size;
};

typedef enum TypeKind { TY_INT, TY_PTR, TY_FUNC } TypeKind;

struct Type {
    TypeKind kind;
    Type *base;
    Token *name;
    Type *ret_type;
    Type *params;
    Type *next;
};

struct Node {
    Type *type;

    NodeKind kind;
    Node *next;
    Node *lhs;
    Node *rhs;
    int val;
    Object *var;
    Node *body;

    // if | for
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    Token *tok;
    char *func_name;
    Node *args;
};

extern Type *TypeInt;

bool is_int(Type *type);
bool is_ptr(Type *type);
void add_type(Node *nd);
Type *func_type(Type *ret_type);
Type *copy_type(Type *type);

Type *pointer_to(Type *base);

Function *parse(Token *tok);

void codegen(Function *nd);