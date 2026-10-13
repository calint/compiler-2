#!/bin/sh
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TREESITTER="../../../../tree-sitter-baz"

if [ ! -d "$TREESITTER" ]; then
	echo "missing $TREESITTER"
	exit 1
fi

LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT
# print the captured output of a failed step
fail() {
	cat "$LOG"
	exit 1
}

tree-sitter generate >"$LOG" 2>&1 || fail
tree-sitter test >"$LOG" 2>&1 || fail
cc -o baz.so -shared src/parser.c -I./node_modules/tree-sitter/lib/include -fPIC >"$LOG" 2>&1 || fail
mv -f baz.so ~/.local/share/nvim/site/parser/
cp queries/highlights.scm ~/.local/share/nvim/site/queries/baz/highlights.scm
# symbols for aerial.nvim
cp queries/aerial.scm ~/.local/share/nvim/site/queries/baz/aerial.scm
# the highlights query needs the '#baz-global?' predicate from the plugin
mkdir -p ~/.local/share/nvim/site/plugin
cp plugin/baz-globals.lua ~/.local/share/nvim/site/plugin/
# in-process language server for symbols, definition, references and rename
cp plugin/baz-lsp.lua ~/.local/share/nvim/site/plugin/
# copy to `tree-sitter-baz` project
cp grammar.js $TREESITTER/
cp README.md $TREESITTER/
cp queries/highlights.scm $TREESITTER/queries/
cp queries/aerial.scm $TREESITTER/queries/
mkdir -p $TREESITTER/plugin
cp plugin/baz-globals.lua $TREESITTER/plugin/
cp plugin/baz-lsp.lua $TREESITTER/plugin/

echo ok
