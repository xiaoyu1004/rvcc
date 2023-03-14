#include "rvcc.h"

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

static bool str_equal(Token *tok, const char *str) {
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
 *      program = stmt*
 *      stmt = express_stmt
 *      express_stmt = expr ;
 *      expr = equality
 *      equality = relational (==relational | !=relational)*
 *      relational = add (<add | <=add | >add | >=add)*
 *      add = mul (+mul | -mul)*
 *      mul = unary (*unary | /unary)*
 *      unary = (+ | -)(unary | primary)
 *      primary = expr | num
 */
static Node *primary(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);

// primary = (expr) | num
static Node *primary(Token **rest, Token *tok) {
    // (expr)
    if (str_equal(tok, "(")) {
        Node *nd = expr(&tok, tok->next);
        *rest    = skip(tok, ")");
        return nd;
    } else if (tok->kind == TK_NUM) {
        Node *nd = new_num(tok->val);
        *rest    = tok->next;
        return nd;
    } else {
        error_tok(tok, "expected a expression or a num");
    }
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

// expr = equality
static Node *expr(Token **rest, Token *tok) { return equality(rest, tok); }

static Node *express_stmt(Token **rest, Token *tok) {
    Node *nd = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest    = skip(tok, ";");
    return nd;
}

static Node *stmt(Token **rest, Token *tok) { return express_stmt(rest, tok); }

static Node *program(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    while (tok->kind != TK_EOF) {
        cur->next = stmt(&tok, tok);
        cur       = cur->next;
    }
    *rest = tok;
    return head.next;
}

Node *parse(Token *tok) {
    Node *nd = program(&tok, tok);
    if (tok->kind != TK_EOF) {
        error_tok(tok, "extra token");
    }
    return nd;
}