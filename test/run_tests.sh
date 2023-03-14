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

assert 0 '0;'
assert 42 '42;'

# +-*/
assert 2 '1-2+3;'
assert 41 '12+34-5;'
assert 17 '1-8/(2*2)+3*6;'
assert 47 '7-8/(2*2)+3*6-3+(3)*(4+5);'

# -- ++
assert 1 '-2+--3;'
assert 13 '--2++--+3+--8;'
assert 17 '--8--9;'

# == != < <= > >=
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'
assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'
assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'
assert 1 '5==2+3;'
assert 0 '6==4+3;'
assert 1 '0*9+5*2==4+4*(6/3)-2;'

# ;
assert 3 '1;2;3;'
assert 12 '12+23;12+99/3;78-66;'

echo OK