clang++ -std=c++14 -o compi src/main.cpp &&
./compi > gen.s &&
nasm -f elf64 gen.s && 
ld -s -o binary gen.o &&
echo && ls --color -la binary && echo && 
./binary

