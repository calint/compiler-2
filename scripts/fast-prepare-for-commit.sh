#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ..

# qa/lint/fix-source.py --apply
echo clang tidy
qa/lint/clang-tidy.sh fix
echo organize source
qa/lint/format-source.py --apply
echo clang format
clang-format -i --style=file src/*
echo build with -O3
./make.sh build -O3
echo run tests
qa/coverage/test-coverage.sh --target=x86 run
qa/coverage/test-coverage.sh --target=rv32i run
qa/coverage/test-coverage.sh --target=rv32i-qemu run
qa/coverage/test-coverage.sh --target=rv32i-fpga run
echo compile prog
./run.sh
echo compile roome
./run-roome.sh
echo test roome
etc/roome/test.sh
echo make readme
etc/readme/make.sh
