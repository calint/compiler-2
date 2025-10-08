#!/bin/sh
# tools:
#   clang++: 18.1.8
#       g++: 14.2.0
#      nasm: 2.16.03
#        ld: 2.43
set -e
cd $(dirname "$0")

# set environment
CC="clang++ -std=c++23"
CW="-Weverything -Wno-c++98-compat -Wno-pre-c++20-compat-pedantic -Wno-weak-vtables -Wno-padded"

#CC="g++ -std=c++23"
#CW="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow"

CF="-Wfatal-errors -Werror"
MSAN=
PROF=
DBG=-g
OPT=
#OPT=-O3
if [ "$1" = "msan" ]; then
  MSAN="-fsanitize=address,undefined"
  DBG=-g
elif [ "$1" = "prof-msan" ]; then
  # profiling, sanitize (note: sanitize crashes unpredictably with core dump)
  #    MSAN="-fsanitize=address,undefined -fno-omit-frame-pointer"
  PROF="-fprofile-instr-generate -fcoverage-mapping"

  # UBSAN_OPTIONS=print_stacktrace=1 ./baz 2>&1 | tee error.log
  #
  # -fsanitize=undefined,integer
  MSAN="-fsanitize=address,leak,nullability \
    -fstack-protector-strong -fno-omit-frame-pointer -fno-optimize-sibling-calls \
    -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_ENABLE_ASSERTIONS=1"
  DBG=-g
fi

SEP="--------------------------------------------------------------------------------"

CMD="$CC src/main.cpp -o baz $OPT $DBG $CF $CW $MSAN $PROF"
echo $SEP
echo $CMD
$CMD
echo $SEP

if [ "$1" = "build" ]; then
  exit 0
elif [ "$1" = "prof-msan" ]; then
  exit 0
fi

./baz prog.baz >gen.s
grep -v -e'^\s*;.*$' -e'^\s*$' gen.s >gen-without-comments.s
nasm -f elf64 gen.s
ld -s -o gen gen.o

ls --color -la baz gen.s gen-without-comments.s gen

echo $SEP
echo '              lines   words   chars'
echo -n '    source: '
cat src/* | wc
echo -n '   gzipped: '
cat src/* | gzip | wc
echo -n 'baz source: '
cat prog.baz | grep -v -e'^\s*$' | wc
echo -n '   gzipped: '
cat prog.baz | grep -v -e'^\s*$' | gzip | wc
echo -n 'asm source: '
cat gen-without-comments.s | wc
echo -n '   gzipped: '
cat gen-without-comments.s | gzip | wc
echo $SEP
./gen
echo $SEP
