#include "rvcc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments, require 2 arguments, but provided %d", argv[1], argc);
        return -1;
    }

    // char *str = "a=3;a;";
    // Lexical analysis, generate token
    Token *tok = tokenize(argv[1]);

    // build ast
    Function *prog = parse(tok);

    // codegen
    codegen(prog);

    return 0;
}