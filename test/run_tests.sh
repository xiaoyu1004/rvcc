#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    cd /home/yuhongzhang/workspace/rvcc/build
    ./rvcc "$input" > tmp.s || exit
    riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 tmp.s -o tmp
    qemu-riscv32 /home/yuhongzhang/workspace/rvcc/build/tmp

    actual="$?"
    if [ "$expected" == "$actual" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, bug got $actual"
    fi
}

# assert 0 'int main(){return 0;}'
# assert 42 'int main(){return 42;}'

# # +-*/
# assert 2 'int main(){return 1-2+3;}'
# assert 41 'int main(){return 12+34-5;}'
# assert 17 'int main(){return 1-8/(2*2)+3*6;}'
# assert 47 'int main(){return 7-8/(2* 2)+3*6-3+(3)*(4+5);}'

# # -- ++
# assert 1 'int main()  {return -2+--3;}'
# assert 13 'int main(){return --2++--+3+--8;}'
# assert 17 'int main(){return --8--9;}'

# # == != < <= > >=
# assert 0 'int main(){return 0==1;}'
# assert 1 'int main(){return 42==42;}'
# assert 1 'int main(){return 0!=1;}'
# assert 0 'int main(){return 42!=42;}'
# assert 1 'int main(){return 0<1;}'
# assert 0 'int main(){return 1<1;}'
# assert 0 'int main(){return 2<1;}'
# assert 1 'int main(){return 0<=1;}'
# assert 1 'int main(){return 1<=1;}'
# assert 0 'int main(){return 2<=1;}'
# assert 1 'int main(){return 1>0;}'
# assert 0 'int main(){return 1>1;}'
# assert 0 'int main(){return 1>2;}'
# assert 1 'int main(){return 1>=0;}'
# assert 1 'int main(){return 1>=1;}'
# assert 0 'int main(){return 1>=2;}'
# assert 1 'int main(){return 5==2+3;}'
# assert 0 'int main(){return 6==4+3;}'
# assert 1 'int main(){return 0*9+5*2==4+4*(6/3)-2;}'

# # ;
# assert 3 'int main(){1;2;return 3;}'
# assert 12 'int main(){12+23;12+99/3;return 78-66;}'

# # single alphabet
# assert 3 "int main(){int a = 3; return a;}"
# assert 8 'int main(){int a=3; int z=5;return a+z;}'
# assert 6 'int main(){int a=3, b=3; return a+b;}'
# assert 5 'int main(){int a=3; int b=4;int a=1; return a+b;}'

# # multi alphabet
# assert 3 'int main(){int foo=3; return foo;}'
# assert 74 'int main(){int foo2=70; int bar4=4; return foo2 + bar4;}'

# # return
# assert 1 'int main(){return 1; 2; 3;}'
# assert 2 'int main(){1; return 2; 3;}'
# assert 3 'int main(){1; 2; return 3; }'

# # {}
# assert 1 'int main(){ return 1; 2; 3; }'
# assert 2 'int main(){ 1; return 2; 3; }'
# assert 3 'int main(){ 1; 2; return 3; }'

# # ;
# assert 1 'int main(){ ;;;; }'
# assert 2 'int main(){ ;;;; return 2;}'

# # if else
# assert 3 'int main(){ if (0) return 2; return 3; }'
# assert 3 'int main(){ if (1-1) return 2; return 3; }'
# assert 2 'int main(){ if (1) return 2; return 3; }'
# assert 2 'int main(){ if (2-1) return 2; return 3; }'
# assert 4 'int main(){ if (0) { 1; 2; return 3; } else { return 4; } }'
# assert 3 'int main(){ if (1) { 1; 2; return 3; } else { return 4; } }'

# # for
# assert 55 'int main(){int i=0; int j=0; for ( i=0; i<=10; i=i+1) int j=i+j; return j; }'
# assert 3 'int main(){ for (;;) {return 3;} return 5; }'

# # while
# assert 10 'int main(){int i=0; while(i<10) { i=i+1; } return i; }'

# # & *
# assert 3 'int main(){int x=3; return *&x; }'
# assert 3 'int main(){int x=3; int * y=&x; int** z=&y; return **z; }'
# assert 5 'int main(){int x=3; int y=5; return *(&x+1); }'
# assert 3 'int main(){int x=3; int y=5; return *(&y-1); }'
# assert 5 'int main(){int x=3; int *y=&x; *y=5; return x; }'
# assert 7 'int main(){int x=3; int y=5; *(&x+1)=7; return y; }'
# assert 7 'int main(){int x=3; int y=5; *(&y-1)=7; return x; }'

# # Pointer arithmetic operation
# assert 3 'int main(){int x=3; int y=5; return *(&y-1); }'
# assert 5 'int main(){int x=3; int y=5; return *(&x+1); }'
# assert 7 'int main(){int x=3; int y=5; *(&y-1)=7; return x; }'
# assert 7 'int main(){int x=3; int y=5; *(&x+1)=7; return y; }'
# assert 6 "int main(){ int x = 3, y = 6,  z; return y; }"

# # definition function && call function
# assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
# assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
# assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

echo OK