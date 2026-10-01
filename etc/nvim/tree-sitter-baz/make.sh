#!/bin/sh
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TREESITTER="../../../../tree-sitter-baz"

echo generate
tree-sitter generate
cc -o baz.so -shared src/parser.c -I./node_modules/tree-sitter/lib/include -fPIC
mv -f baz.so ~/.local/share/nvim/site/parser/
echo copy to nvim config
cp queries/highlights.scm ~/.local/share/nvim/site/queries/baz/highlights.scm
# symbols for aerial.nvim
cp queries/aerial.scm ~/.local/share/nvim/site/queries/baz/aerial.scm
# the highlights query needs the '#baz-global?' predicate from the plugin
mkdir -p ~/.local/share/nvim/site/plugin
cp plugin/baz-globals.lua ~/.local/share/nvim/site/plugin/
# copy to `tree-sitter-baz` project
echo copy to $(realpath $TREESITTER)
cp grammar.js $TREESITTER/
cp README.md $TREESITTER/
cp queries/highlights.scm $TREESITTER/queries/
cp queries/aerial.scm $TREESITTER/queries/
mkdir -p $TREESITTER/plugin
cp plugin/baz-globals.lua $TREESITTER/plugin/
