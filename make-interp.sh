mkdir -p bin
g++ -o bin/vm -std=c++20 ./asm.interpreter.cc ./machine.cc -O0 -g -I$PWD
