#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ..

echo clang tidy
qa/lint/clang-tidy.sh fix
echo format source
qa/lint/format-source.sh
echo test lsp
etc/nvim/tree-sitter-baz/qa/test.sh
echo test roome
roome/qa/test.sh
echo coverage all
qa/coverage/test-all.sh
echo make
./make.sh build
echo compile prog
./run.sh
echo make readme
etc/readme/make.sh
