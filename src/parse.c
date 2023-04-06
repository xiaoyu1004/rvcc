#include "rvcc.h"

Object *g_locals = NULL;

static char *get_ident(const Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expect a variable name");
    }
    return strndup(tok->loc, tok->len);
}

static bool is_var_decl(Token *tok) {
    static char *types[] = {"int"};
    for (size_t i = 0; i < sizeof(types) / sizeof(*types); ++i) {
        return str_equal(tok, types[i]);
    }
    return false;
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *nd = calloc(1, sizeof(Node));
    nd->kind = kind;
    nd->tok  = tok;

    // init pointer
    nd->next = NULL;
    nd->lhs  = NULL;
    nd->rhs  = NULL;
    nd->body = NULL;
    nd->init = NULL;
    nd->cond = NULL;
    nd->inc  = NULL;
    return nd;
}

static Node *new_num(int val, Token *tok) {
    Node *nd = new_node(ND_NUM, tok);
    nd->val  = val;
    return nd;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *nd = new_node(kind, tok);
    nd->lhs  = expr;
    nd->rhs  = NULL;
    return nd;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *nd = new_node(kind, tok);
    nd->lhs  = lhs;
    nd->rhs  = rhs;
    return nd;
}

static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_int(lhs->type) && is_int(rhs->type)) {
        return new_binary(ND_ADD, lhs, rhs, tok);
    }

    // ptr + ptr
    if (is_ptr(lhs->type) && is_ptr(rhs->type)) {
        error_tok(tok, "Both addends cannot be of pointer type");
    }

    // num + ptr -> ptr + num
    if (is_int(lhs->type) && is_ptr(rhs->type)) {
        Node *tmp = lhs;
        lhs       = rhs;
        rhs       = tmp;
    }

    rhs = new_binary(ND_MUL, rhs, new_num(REG_BYTES, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_int(lhs->type) && is_int(rhs->type)) {
        return new_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - ptr
    if (is_ptr(lhs->type) && is_ptr(rhs->type)) {
        error_tok(tok, "Both addends cannot be of pointer type");
    }

    // num - ptr -> ptr - num
    if (is_int(lhs->type) && is_ptr(rhs->type)) {
        Node *tmp = lhs;
        lhs       = rhs;
        rhs       = tmp;
    }

    rhs = new_binary(ND_MUL, rhs, new_num(-REG_BYTES, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node *new_var_node(Object *obj, Token *tok) {
    Node *nd = new_node(ND_VAR, tok);
    nd->var  = obj;
    return nd;
}

static Object *new_var_object(const char *name, Type *type) {
    Object *obj = calloc(1, sizeof(Object));
    obj->name   = name;
    obj->next   = g_locals;
    obj->type   = type;
    g_locals    = obj;
    return obj;
}

static Object *find_var(Token *tok) {
    for (Object *var = g_locals; var != NULL; var = var->next) {
        if (strlen(var->name) == tok->len && !strncmp(var->name, tok->loc, tok->len)) {
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
 *      program = function*
 *      function = declspec declarator compound_stmt
 *      compound_stmt = { (stmt)* }
 *      stmt = (return expr;) |
 *              if (expr) stmt (else stmt)? |
 *              for (express_stmt expr? expr?) stmt |
 *              while (expr) stmt |
 *              compound_stmt |
 *              express_stmt |
 *              declaration
 * declaration = declspec ( declarator (=expr)? (, declarator (=expr)?)*)?;
 * declspec = int
 * declarator = "*"* ident type_suffix
 * type_suffix = ("()")?
 *
 * express_stmt = expr?;
 * expr = assign
 * assign = equality (=assign)? equality = relational (==relational | !=relational)*
 * relational = add (<add | <=add | >add | >=add)*
 * add = mul (+mul | -mul)*
 * mul = unary (*unary | /unary)*
 * unary = (+ | - | & | *)(unary | primary)
 * primary = expr | num | ident | func_call
 * func_call = ident( (expr(,expr)*)? )
 */
static Node *primary(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Type *declspec(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *);
static Node *declaration(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Function *function(Token **rest, Token *tok);
static Function *program(Token **rest, Token *tok);

// func_call = ident( (expr(,expr)*)? )
static Node *func_call(Token **rest, Token *tok) {
    Node *nd      = new_node(ND_FUNCCALL, tok);
    nd->func_name = strndup(tok->loc, tok->len);
    tok = skip(tok->next, "(");

    Node head = {};
    Node *cur = &head;
    while (!str_equal(tok, ")")) {
        if (cur != &head) {
            tok = skip(tok, ",");
        }
        cur->next = expr(&tok, tok->next->next);
        cur       = cur->next;
    }
    *rest    = skip(tok, ")");
    nd->args = head.next;
    return nd;
}

// primary = (expr) | num
static Node *primary(Token **rest, Token *tok) {
    // (expr)
    if (str_equal(tok, "(")) {
        Node *nd = expr(&tok, tok->next);
        *rest    = skip(tok, ")");
        return nd;
    }
    if (tok->kind == TK_NUM) {
        Node *nd = new_num(tok->val, tok);
        *rest    = tok->next;
        return nd;
    }
    if (tok->kind == TK_IDENT) {
        // func call
        if (str_equal(tok->next, "(")) {
            return func_call(rest, tok);
        }

        Object *var = find_var(tok);
        if (var == NULL) {
            error_tok(tok, "undefine variable");
        }
        Node *nd = new_var_node(var, tok);
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
        nd = new_unary(ND_NEG, unary(rest, tok->next), tok);
    } else if (str_equal(tok, "&")) {
        nd = new_unary(ND_ADDR, unary(rest, tok->next), tok);
    } else if (str_equal(tok, "*")) {
        nd = new_unary(ND_DEREF, unary(rest, tok->next), tok);
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
        Token *start = tok;
        if (str_equal(tok, "*")) {
            nd = new_binary(ND_MUL, nd, unary(&tok, tok->next), start);
        } else if (str_equal(tok, "/")) {
            nd = new_binary(ND_DIV, nd, unary(&tok, tok->next), start);
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
        Token *start = tok;
        if (str_equal(tok, "+")) {
            nd = new_add(nd, mul(&tok, tok->next), start);
        } else if (str_equal(tok, "-")) {
            nd = new_sub(nd, mul(&tok, tok->next), start);
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
        Token *start = tok;
        if (str_equal(tok, "<")) {
            nd = new_binary(ND_LT, nd, add(&tok, tok->next), start);
        } else if (str_equal(tok, "<=")) {
            nd = new_binary(ND_LE, nd, add(&tok, tok->next), start);
        } else if (str_equal(tok, ">")) {
            nd = new_binary(ND_LT, add(&tok, tok->next), nd, start);
        } else if (str_equal(tok, ">=")) {
            nd = new_binary(ND_LE, add(&tok, tok->next), nd, start);
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
        Token *start = tok;
        if (str_equal(tok, "==")) {
            nd = new_binary(ND_EQ, nd, relational(&tok, tok->next), start);
        } else if (str_equal(tok, "!=")) {
            nd = new_binary(ND_NE, nd, relational(&tok, tok->next), start);
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
        Token *start = tok;
        nd           = new_binary(ND_ASSIGN, nd, assign(&tok, tok->next), start);
    }
    *rest = tok;
    return nd;
}

// expr = equality
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

static Node *express_stmt(Token **rest, Token *tok) {
    // ;
    if (str_equal(tok, ";")) {
        Node *nd = new_node(ND_BLOCK, tok);
        *rest    = tok->next;
        return nd;
    }

    // expr;
    Node *nd = new_node(ND_EXPR_STMT, tok);
    nd->lhs  = expr(&tok, tok);
    *rest    = skip(tok, ";");
    return nd;
}

// declaration = declspec ( declarator (=expr)? (, declarator (=expr)?)* )?;
static Node *declaration(Token **rest, Token *tok) {
    Type *base_type = declspec(&tok, tok);

    Node head = {};
    Node *cur = &head;
    // count var
    int i = 0;
    while (!str_equal(tok, ";")) {
        if (i++ > 0) {
            tok = skip(tok, ",");
        }
        Type *type  = declarator(&tok, tok, base_type);
        Object *var = new_var_object(get_ident(type->name), type);

        if (!str_equal(tok, "=")) {
            continue;
        }

        Node *lhs = new_var_node(var, type->name);
        Node *rhs = expr(&tok, tok->next);
        Node *nd  = new_binary(ND_ASSIGN, lhs, rhs, tok);

        cur->next = new_unary(ND_EXPR_STMT, nd, tok);
        cur       = cur->next;
    }

    Node *nd = new_node(ND_BLOCK, tok);
    nd->body = head.next;
    *rest    = skip(tok, ";");
    return nd;
}

// declspec = int
static Type *declspec(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return TypeInt;
}

static Type *type_suffix(Token **rest, Token *tok, Type *type) {
    // type_suffix
    if (str_equal(tok, "(")) {
        *rest = skip(tok->next, ")");
        return func_type(type);
    }

    *rest = tok;
    return type;
}

// declarator = "*"* ident type_suffix
// type_suffix = ("()")?
static Type *declarator(Token **rest, Token *tok, Type *type) {
    while (consume(&tok, tok, "*")) {
        type = pointer_to(type);
    }

    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expect a variable name");
    }
    // ident
    type->name = tok;
    return type_suffix(rest, tok->next, type);
}

static Node *stmt(Token **rest, Token *tok) {
    // return
    if (str_equal(tok, "return")) {
        Node *nd = new_node(ND_RETURN, tok);
        nd->lhs  = expr(&tok, tok->next);
        *rest    = skip(tok, ";");
        return nd;
    }

    // if (expr) stmt (else stmt)?
    if (str_equal(tok, "if")) {
        tok      = skip(tok->next, "(");
        Node *nd = new_node(ND_IF, tok);
        nd->cond = expr(&tok, tok);
        tok      = skip(tok, ")");
        nd->then = stmt(&tok, tok);
        // else
        if (str_equal(tok, "else")) {
            nd->els = stmt(&tok, tok->next);
        }
        *rest = tok;
        return nd;
    }

    // for (express_stmt expr? expr?) stmt
    if (str_equal(tok, "for")) {
        tok      = skip(tok->next, "(");
        Node *nd = new_node(ND_FOR, tok);
        nd->init = express_stmt(&tok, tok);
        if (!str_equal(tok, ";")) {
            nd->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");
        if (!str_equal(tok, ")")) {
            nd->inc = expr(&tok, tok);
        }
        tok      = skip(tok, ")");
        nd->then = stmt(&tok, tok);
        *rest    = tok;
        return nd;
    }

    // while (expr) stmt
    if (str_equal(tok, "while")) {
        Node *nd = new_node(ND_FOR, tok);
        tok      = skip(tok->next, "(");
        nd->cond = expr(&tok, tok);
        tok      = skip(tok, ")");
        nd->then = stmt(&tok, tok);
        *rest    = tok;
        return nd;
    }

    // compound_stmt
    if (str_equal(tok, "{")) {
        return compound_stmt(rest, tok);
    }

    // variable declaration
    if (is_var_decl(tok)) {
        return declaration(rest, tok);
    }

    return express_stmt(rest, tok);
}

// compound_stmt = { stmt* }
static Node *compound_stmt(Token **rest, Token *tok) {
    Node *nd = new_node(ND_BLOCK, tok);

    Node head = {};
    Node *cur = &head;
    tok       = skip(tok, "{");
    while (!str_equal(tok, "}")) {
        cur->next = stmt(&tok, tok);
        cur       = cur->next;
        // add type
        add_type(cur);
    }
    tok = skip(tok, "}");

    nd->body = head.next;

    *rest = tok;
    return nd;
}

// declspec declarator compound_stmt
static Function *function(Token **rest, Token *tok) {
    Type *base_type = declspec(&tok, tok);
    Type *type      = declarator(&tok, tok, base_type);

    g_locals         = NULL;
    Function *func   = calloc(1, sizeof(Function));
    func->name       = get_ident(type->name);
    func->body       = compound_stmt(rest, tok);
    func->locals     = g_locals;
    func->stack_size = 0;
    return func;
}

static Function *program(Token **rest, Token *tok) {
    Function head;
    Function *cur = &head;
    while (tok->kind != TK_EOF) {
        cur->next = function(&tok, tok);
        cur       = cur->next;
    }
    *rest = tok;
    return head.next;
}

Function *parse(Token *tok) {
    Function *func = program(&tok, tok);
    if (tok->kind != TK_EOF) {
        error_tok(tok, "extra token");
    }
    return func;
}