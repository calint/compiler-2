#!/bin/sh
# formats the sources under 'src/': clang-format first so that the blank lines
# of AGENTS.md see the final shape of each signature, then the layout of the
# classes and those blank lines by 'format-source.py', then clang-format again
# usage: qa/lint/format-source.sh
set -eu
cd "$(dirname "$0")/../.."

echo "format source: clang-format"
clang-format -i --style=file src/*

echo "format source: class layout and blank lines"
qa/lint/format-source.py --apply

echo "format source: clang-format"
clang-format -i --style=file src/*

echo "format source: done"
