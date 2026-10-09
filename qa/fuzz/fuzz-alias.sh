#!/bin/sh
# fuzzes the aliasing check ('--checks=alias') against a brute force of the
# bytes: the path comparison alone (C++), then random programs with the element
# 'e' of a 'foo' and with 'mut' parameters through the compiler (needs a built
# '../../baz', see 'make.sh build')
# usage: qa/fuzz/fuzz-alias.sh [paths] [programs] [seed]
#   paths     pairs of accesses for the C++ fuzz (default 2000000)
#   programs  programs for the compiler fuzz (default 5000)
#   seed      random seed (default 1)
set -eu
cd "$(dirname "$0")"

PATHS="${1:-2000000}"
PROGRAMS="${2:-5000}"
SEED="${3:-1}"

TMP="$(mktemp -d /tmp/fuzz-alias.XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

echo "alias fuzz: path comparison"
clang++ -std=c++26 -O2 -Wno-braced-scalar-init alias-path-fuzz.cpp \
    -o "$TMP/alias-path-fuzz"
"$TMP/alias-path-fuzz" "$PATHS" "$SEED"

echo "alias fuzz: programs"
./alias-foo-fuzz.py "$PROGRAMS" "$SEED" ../../baz

echo "alias fuzz: done"
