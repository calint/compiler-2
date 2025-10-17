#!/bin/sh
set -e
tree-sitter generate
cc -o baz.so -shared src/parser.c -I./node_modules/tree-sitter/lib/include -fPIC
mv -f baz.so ~/.local/share/nvim/site/parser/
cp queries/highlights.scm ~/.local/share/nvim/site/queries/baz/highlights.scm
