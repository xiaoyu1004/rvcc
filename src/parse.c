#include "rvcc.h"

Object *g_locals = NULL;

static Node *new_node(NodeKind kind) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->next = NULL;
    nd->lhs  = NULL;
    nd->rhs  = NULL;
    return nd;
}

static Node *new_num(int val) {
    Node *nd = new_node(ND_NUM);
    nd->val  = val;
    return nd;
}

static Node *new_unary(NodeKind kind, Node *expr) {
    Node *nd = new_node(kind);
    nd->lhs  = expr;
    nd->rhs  = NULL;
    return nd;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *nd = new_node(kind);
    nd->lhs  = lhs;
    nd->rhs  = rhs;
    return nd;
}

static Node *new_var_node(Object *obj) {
    Node *nd = new_node(ND_VAR);
    nd->var  = obj;
    return nd;
}

static Object *new_var_object(char *name) {
    Object *obj = calloc(1, sizeof(Object));
    obj->name   = name;
    obj->next   = g_locals;
    g_locals    = obj;
    return obj;
}

static Object *find_var(Token *tok) {
    for (Object *var = g_locals; var != NULL; var = var->next) {
        if (strlen(var->name) == tok->len &&
            !strncmp(var->name, tok->loc, tok->len)) {
            return var;
        }
    }
    return NULL;
}

bool str_equal(Token *tok, const char *str) {
    return memcmp(tok->loc, str, tok->len) == 0 && str[tok->len] == '\0';
}

static Token *skip(Token *tok, const char *str) {
    if (!str_equal(tok, str)) {
        error_tok(tok, "expected %s", str);
    }
    return tok->next;
}

/**
 * Derived formula:
 *      program = compound_stmt
 *      compound_stmt = { stmt* }
 *      stmt = (return expr;) | compound_stmt | express_stmt
 *      express_stmt = expr;
 *      expr = assign
 *      assign = equality (=assign)?
 *      equality = relational (==relational | !=relational)*
 *      relational = add (<add | <=add | >add | >=add)*
 *      add = mul (+mul | -mul)*
 *      mul = unary (*unary | /unary)*
 *      unary = (+ | -)(unary | primary)
 *      primary = expr | num | ident
 */
static Node *primary(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *program(Token **rest, Token *tok);

// primary = (expr) | num
static Node *primary(Token **rest, Token *tok) {
    // (expr)
    if (str_equal(tok, "(")) {
        Node *nd = expr(&tok, tok->next);
        *rest    = skip(tok, ")");
        return nd;
    }
    if (tok->kind == TK_NUM) {
        Node *nd = new_num(tok->val);
        *rest    = tok->next;
        return nd;
    }
    if (tok->kind == TK_IDENT) {
        Object *var = find_var(tok);
        if (var == NULL) {
            var = new_var_object(strndup(tok->loc, tok->len));
        }
        Node *nd = new_var_node(var);
        *rest    = tok->next;
        return nd;
    }
    error_tok(tok, "expected a expression or a num");
}

// unary = (+ | -)unary | primary
static Node *unary(Token **rest, Token *tok) {
    Node *nd = NULL;
    if (str_equal(tok, "+")) {
        nd = unary(rest, tok->next);
    } else if (str_equal(tok, "-")) {
        nd = new_unary(ND_NEG, unary(rest, tok->next));
    } else {
        nd = primary(rest, tok);
    }
    return nd;
}

// mul = unary (*unary | /unary)
static Node *mul(Token **rest, Token *tok) {
    // parse primary
    Node *nd = unary(&tok, tok);
    // parse binary operator
    while (true) {
        if (str_equal(tok, "*")) {
            nd = new_binary(ND_MUL, nd, unary(&tok, tok->next));
        } else if (str_equal(tok, "/")) {
            nd = new_binary(ND_DIV, nd, unary(&tok, tok->next));
        } else {
            *rest = tok;
            return nd;
        }
    }
}

// add = mul (+mul | -mul)*
static Node *add(Token **rest, Token *tok) {
    // parse mul
    Node *nd = mul(&tok, tok);
    // +mul | -mul
    while (true) {
        if (str_equal(tok, "+")) {
            nd = new_binary(ND_ADD, nd, mul(&tok, tok->next));
        } else if (str_equal(tok, "-")) {
            nd = new_binary(ND_SUB, nd, mul(&tok, tok->next));
        } else {
            break;
        }
    }
    *rest = tok;
    return nd;
}

// relational = add (<add | <=add | >add | >=add)*
static Node *relational(Token **rest, Token *tok) {
    Node *nd = add(&tok, tok);
    while (true) {
        if (str_equal(tok, "<")) {
            nd = new_binary(ND_LT, nd, add(&tok, tok->next));
        } else if (str_equal(tok, "<=")) {
            nd = new_binary(ND_LE, nd, add(&tok, tok->next));
        } else if (str_equal(tok, ">")) {
            nd = new_binary(ND_LT, add(&tok, tok->next), nd);
        } else if (str_equal(tok, ">=")) {
            nd = new_binary(ND_LE, add(&tok, tok->next), nd);
        } else {
            break;
        }
    }
    *rest = tok;
    return nd;
}

// equality = relational (==relational | !=relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *nd = relational(&tok, tok);
    while (true) {
        if (str_equal(tok, "==")) {
            nd = new_binary(ND_EQ, nd, relational(&tok, tok->next));
        } else if (str_equal(tok, "!=")) {
            nd = new_binary(ND_NE, nd, relational(&tok, tok->next));
        } else {
            break;
        }
    }
    *rest = tok;
    return nd;
}

static Node *assign(Token **rest, Token *tok) {
    Node *nd = equality(&tok, tok);
    if (str_equal(tok, "=")) {
        nd = new_binary(ND_ASSIGN, nd, assign(&tok, tok->next));
    }
    *rest = tok;
    return nd;
}

// expr = equality
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

static Node *express_stmt(Token **rest, Token *tok) {
    Node *nd = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest    = skip(tok, ";");
    return nd;
}

static Node *stmt(Token **rest, Token *tok) {
    if (str_equal(tok, "return")) {
        Node *nd = new_unary(ND_RETURN, expr(&tok, tok->next));
        *rest    = skip(tok, ";");
        return nd;
    }
    if (str_equal(tok, "{")) {
        return compound_stmt(rest, tok);
    }
    return express_stmt(rest, tok);
}

// compound_stmt = { stmt* }
static Node *compound_stmt(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    tok       = skip(tok, "{");
    while (!str_equal(tok, "}")) {
        cur->next = stmt(&tok, tok);
        cur       = cur->next;
    }
    tok = skip(tok, "}");

    Node *nd = new_node(ND_BLOCK);
    nd->body = head.next;

    *rest = tok;
    return nd;
}

static Node *program(Token **rest, Token *tok) {
    return compound_stmt(rest, tok);
}

Function *parse(Token *tok) {
    Node *nd = program(&tok, tok);
    if (tok->kind != TK_EOF) {
        error_tok(tok, "extra token");
    }

    Function *prog   = calloc(1, sizeof(Function));
    prog->body       = nd;
    prog->locals     = g_locals;
    prog->stack_size = 0;
    return prog;
}