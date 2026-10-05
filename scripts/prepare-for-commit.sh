#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ..

echo clang tidy
qa/lint/clang-tidy.sh fix
echo format source
qa/lint/format-source.sh
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
