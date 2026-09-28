#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ..

# echo fix const on everything
# qa/lint/fix-source.py --apply
echo clang tidy
qa/lint/clang-tidy.sh fix
echo organize source
qa/lint/format-source.py --apply
echo clang format
clang-format -i --style=file src/*
echo coverage all
qa/coverage/test-all.sh
echo make
./make.sh build
echo compile prog
./run.sh --checks=noub
echo compile roome
./run.sh --checks=noub roome.baz
echo make readme
etc/readme/make.sh
