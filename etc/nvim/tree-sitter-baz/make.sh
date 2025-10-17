rm -rf src
tree-sitter generate
gcc -shared -fPIC -o baz.so src/parser.c
