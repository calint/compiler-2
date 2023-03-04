#for o in s 0 1 2 3;do
CC="clang++ -std=c++2a"
CF="-Os -g3 -Wfatal-errors"
CW="-pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wmissing-declarations -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -Wswitch-default -Wundef -Wfloat-equal -Wsign-conversion -Wfloat-conversion -Wno-float-equal -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wold-style-cast -Wno-unqualified-std-cast-call -Wno-unused-private-field"
for o in s;do
  echo " * optimiziation option: $o " &&
  $CC $CF $CW -O$o -S src/main.cpp &&
  echo -n 'callqs: ' &&
  cat main.s|grep callq|sort -u|wc|awk '{print $1}' &&
  $CC $CF $CW -O$o -Wfatal-errors -o compi src/main.cpp &&
  ls --color -la compi
done
./compi > gen.s &&
nasm -f elf64 gen.s && 
ld -s -o binary gen.o &&
#ls --color -la compi &&
ls --color -la gen.s &&
ls --color -la binary &&
echo &&
echo -n '   compi source: ' && cat src/*|wc &&
echo -n '        gzipped: ' && cat src/*|gzip|wc && 
echo -n '     baz source: ' && cat prog.baz|grep -v -e'^\s*$'|wc &&
echo -n '        gzipped: ' && cat prog.baz|grep -v -e'^\s*$'|gzip|wc &&
echo -n ' x86 asm source: ' && cat src/*|sh pgen.sh|wc &&
echo -n '        gzipped: ' && cat src/*|sh pgen.sh|gzip|wc &&

echo &&
cat gen.s &&
echo &&
./binary

