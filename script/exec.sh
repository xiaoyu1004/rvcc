#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    cd ../build
    ./rvcc $input > tmp.s || exit
    riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 tmp.s -o tmp
    qemu-riscv32 ./tmp

    actual="$?"
    if [ "$expected" == "$actual" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, bug got $actual"
    fi
}

assert 0 0
assert 42 42
assert 2 1-2+3