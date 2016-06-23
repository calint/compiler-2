#for o in s 0 1 2 3;do
for o in 3;do
#  echo " * optimiziation option: $o "
  clang++ -std=c++14 -O$o -S src/main.cpp &&
  echo -n 'callqs: ' 
  cat main.s|grep callq|sort -u|wc|awk '{print $1}' &&
  clang++ -std=c++14 -O$o -o compi src/main.cpp &&
  ls --color -la compi
done
./compi > gen.s &&
nasm -f elf64 gen.s && 
ld -s -o binary gen.o &&
#ls --color -la compi &&
ls --color -la gen.s &&
ls --color -la binary &&
echo &&
echo -n '       src: ' && cat src/*|wc &&
echo -n '   gzipped: ' && cat src/*|gzip|wc &&
echo -n ' generated: ' && sh pgen.sh|wc &&
echo &&
./binary

