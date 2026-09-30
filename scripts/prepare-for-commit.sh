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
./run.sh
echo compile roome
./run-roome.sh
echo test roome
etc/roome/test.sh
echo make readme
etc/readme/make.sh
