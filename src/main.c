#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s: invalid number of arguments!", argv[1]);
        return -1;
    }

    // declare a global main segment, it is also the start of program
    printf("    .global main\n");
    // main label
    printf("main:\n");
    // li is the addi alias instruction, Load an immediate count to the register
    printf("    li a0, %d\n", atoi(argv[1]));
    // ret is the jalr x0, x1, 0 alias instruction, Used to return a subroutine
    printf("    ret\n");
}