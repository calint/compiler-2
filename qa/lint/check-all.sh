#!/bin/sh
# runs every 'check-*.py' of this directory, the design rules that are not
# compiler warnings, e.g. the include layers, the quoting of messages, the
# names of the README, a test for each compile error and the examples of the
# documentation, which need the built '../../baz'; then 'fix-source.py'
# without '--apply' and 'shfmt' over the shell scripts
# usage: qa/lint/check-all.sh
# exits 1 after the first check that fails
set -eu
cd "$(dirname "$0")"

for check in check-*.py; do
    echo "$check"
    "./$check"
done

# the sources have every '[[nodiscard]]' and 'const' that 'fix-source.py' adds
echo "fix-source.py"
./fix-source.py | tee /dev/stderr |
    grep -q "^0 '\[\[nodiscard\]\]' and 0 'const' to add$" || {
    echo "run 'qa/lint/fix-source.py --apply'" >&2
    exit 1
}

# the shell scripts keep the layout of 'shfmt -i 4', the tree-sitter grammar
# has its own
echo "shfmt"
if command -v shfmt >/dev/null; then
    git -C ../.. ls-files '*.sh' ':!etc/nvim' | (cd ../.. && xargs shfmt -i 4 -d)
else
    echo "shfmt: not installed, the layout of the shell scripts is not checked"
fi
