#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ..

echo clang tidy
qa/lint/clang-tidy.sh fix
echo format source
qa/lint/format-source.sh
echo build with -O3
./make.sh build -O3
echo run tests
qa/coverage/test-coverage.sh --target=x86 run
#qa/coverage/test-coverage.sh --target=rv32i run
#qa/coverage/test-coverage.sh --target=rv32i-qemu run
qa/coverage/test-coverage.sh --target=rv32i-fpga run
echo compile prog
./run.sh
echo compile roome
./run-roome.sh
echo test roome
roome/test.sh
echo make readme
etc/readme/make.sh
