#!/bin/sh
# like 'prepare-for-commit.sh' without the coverage build: the cases on
# x86_64 and rv32i-fpga and the command line tests with the '-O3' build
# usage: scripts/fast-prepare-for-commit.sh
# note: './run.sh' runs 'prog.baz', which reads a line of input
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
echo build with -O3
./make.sh build -O3
echo test roome
roome/qa/test.sh
echo run tests
qa/coverage/test-coverage.sh --target=x86 run
#qa/coverage/test-coverage.sh --target=rv32i run
#qa/coverage/test-coverage.sh --target=rv32i-qemu run
qa/coverage/test-coverage.sh --target=rv32i-fpga run
qa/coverage/test-cli.sh
echo compile prog
./run.sh
echo codegen listings
./baz --target=rv32i etc/codegen-inefficiencies.baz >etc/codegen-inefficiencies.s
grep -v -e '^[[:space:]]*#' -e '^[[:space:]]*$' etc/codegen-inefficiencies.s >etc/codegen-inefficiencies-without-comments.s
echo make readme
etc/readme/make.sh
