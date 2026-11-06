#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ..

echo clang tidy
qa/lint/clang-tidy.sh fix
echo format source
qa/lint/format-source.sh
echo check all
qa/lint/check-all.sh
echo test lsp
etc/nvim/tree-sitter-baz/qa/test.sh
echo parse sources
etc/nvim/tree-sitter-baz/qa/parse-sources.sh
echo coverage all
qa/coverage/test-all.sh
echo make
./make.sh build
echo test roome
roome/qa/test.sh
echo compile prog
./run.sh
echo codegen listings
./baz --target=rv32i etc/codegen-inefficiencies.baz >etc/codegen-inefficiencies.s
grep -v -e '^[[:space:]]*#' -e '^[[:space:]]*$' etc/codegen-inefficiencies.s >etc/codegen-inefficiencies-without-comments.s
echo make readme
etc/readme/make.sh
