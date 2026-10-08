#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ..

echo clang tidy
qa/lint/clang-tidy.sh fix
echo format source
qa/lint/format-source.sh
echo coverage all
qa/coverage/test-all.sh
echo make
./make.sh build
echo compile prog
./run.sh
echo compile roome
roome/run.sh
echo test roome
roome/test.sh
echo make readme
etc/readme/make.sh
