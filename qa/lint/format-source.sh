#!/bin/sh
# applies the source formatting rules of AGENTS.md to 'src/', run it before
# clang-format, which removes the blank lines that it does not accept
# usage: qa/lint/format-source.sh
set -eu
cd "$(dirname "$0")/../.."

# the order of the members of a class
qa/lint/format-source.py --apply

# the blank lines
python3 qa/lint/signature-blank.py apply
python3 qa/lint/tight-return.py apply
python3 qa/lint/assert-groups.py apply
python3 qa/lint/multiline-blank.py apply
