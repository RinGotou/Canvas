mkdir -p bin
g++ -o bin/bcvm -std=c++20 ./bytecode.interpreter.cc ./machine.cc -O0 -g -I$PWD
