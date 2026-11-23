#!/bin/sh
# runs every 'check-*.py' of this directory, the design rules that are not
# compiler warnings, e.g. the include layers, the quoting of messages, the
# names of the README, a test for each compile error and the examples of the
# documentation, which need the built '../../baz'; then 'shfmt' over the shell
# scripts
# usage: qa/lint/check-all.sh
# exits 1 after the first check that fails
set -eu
cd "$(dirname "$0")"

for check in check-*.py; do
    echo "$check"
    "./$check"
done

# the shell scripts keep the layout of 'shfmt -i 4', the tree-sitter grammar
# has its own
echo "shfmt"
if command -v shfmt >/dev/null; then
    git -C ../.. ls-files '*.sh' ':!etc/nvim' | (cd ../.. && xargs shfmt -i 4 -d)
else
    echo "shfmt: not installed, the layout of the shell scripts is not checked"
fi
