    .global main
main:
    li a0, 9
    neg a0, a0
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 8
    neg a0, a0
    neg a0, a0
    lw a1, 0(sp)
    addi sp, sp, 4
    sub a0, a0, a1
    ret
