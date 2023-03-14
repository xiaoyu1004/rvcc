#include "rvcc.h"

static char *current_input;

void error(char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char str[1024] = {};
    vsprintf(str, fmt, va);
    fprintf(stderr, "%s\n", str);
    va_end(va);
    exit(1);
}

void verror_at(const char *loc, const char *fmt, va_list va) {
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

void error_at(const char *loc, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    verror_at(loc, fmt, va);
}

void error_tok(const Token *tok, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    verror_at(tok->loc, fmt, va);
}

static int get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

static bool starts_with(const char *str, const char *sub_str) {
    return strncmp(str, sub_str, strlen(sub_str)) == 0;
}

static int read_punct(const char *p) {
    bool is_relation = starts_with(p, "==") || starts_with(p, "!=") ||
                       starts_with(p, ">=") || starts_with(p, "<=");
    return (is_relation ? 2 : 1);
}

static Token *new_token(TokenKind kind, const char *start, const char *end) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind  = kind;
    tok->loc   = start;
    tok->len   = end - start;
    return tok;
}

Token *tokenize(char *p) {
    Token head = {};
    Token *cur = &head;
    while (*p) {
        if (isspace(*p)) {
            p++;
        } else if (isdigit(*p)) {
            cur->next         = new_token(TK_NUM, p, p);
            cur               = cur->next;
            const char *old_p = p;
            cur->val          = strtoul(p, &p, 10);
            cur->len          = p - old_p;
        } else if (ispunct(*p)) {
            int punct_len = read_punct(p);
            cur->next     = new_token(TK_PUNCT, p, p + punct_len);
            cur           = cur->next;
            p += punct_len;
        } else {
            error_at(p, "invalid token\n");
        }
    }

    cur->next = new_token(TK_EOF, p, p);
    return head.next;
}