    .global main
main:
    li a0, 23
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 12
    lw a1, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    li a0, 3
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 99
    lw a1, 0(sp)
    addi sp, sp, 4
    div a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 12
    lw a1, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    li a0, 66
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 78
    lw a1, 0(sp)
    addi sp, sp, 4
    sub a0, a0, a1
    ret
