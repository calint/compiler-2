#!/bin/sh
# formats the sources under 'src/': the layout of the classes and the blank
# lines of AGENTS.md by 'format-source.py', then clang-format
# usage: qa/lint/format-source.sh
set -eu
cd "$(dirname "$0")/../.."

echo "format source: class layout and blank lines"
qa/lint/format-source.py --apply

echo "format source: clang-format"
clang-format -i --style=file src/*

echo "format source: done"
