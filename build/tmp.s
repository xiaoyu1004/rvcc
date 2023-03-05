    .global main
main:
    li a0, 5
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 4
    lw a1, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 3
    lw a1, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 3
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 6
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 3
    lw a1, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 2
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 2
    lw a1, 0(sp)
    addi sp, sp, 4
    mul a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 8
    lw a1, 0(sp)
    addi sp, sp, 4
    div a0, a0, a1
    addi sp, sp, -4
    sw a0, 0(sp)
    li a0, 7
    lw a1, 0(sp)
    addi sp, sp, 4
    sub a0, a0, a1
    lw a1, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    lw a1, 0(sp)
    addi sp, sp, 4
    sub a0, a0, a1
    lw a1, 0(sp)
    addi sp, sp, 4
    add a0, a0, a1
    ret
