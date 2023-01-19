mkdir -p bin
g++ -static -o bin/vm -std=c++20 ./asm.interpreter.cc ./machine.cc -O0 -g -I$PWD
