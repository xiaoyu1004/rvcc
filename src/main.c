#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s: invalid number of arguments!", argv[1]);
        return -1;
    }

    char *p = argv[1];

    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");
    // li is the addi alias instruction, Load an immediate count to the register
    printf("    li a0, %ld\n", strtol(p, &p, 10));

    while (*p)
    {
        if (*p == '+')
        {
            p++;
            printf("    addi a0, a0, %ld\n", strtol(p, &p, 10));
        }
        else if (*p == '-')
        {
            p++;
            printf("    addi a0, a0, -%ld\n", strtol(p, &p, 10));
        }
        else
        {
            fprintf(stderr, "unexpected character: '%c'\n", *p);
        }
    }

    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");
}