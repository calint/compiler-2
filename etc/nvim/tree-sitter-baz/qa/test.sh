#!/bin/sh
# tests the baz language server, needs the parser installed by ../make.sh
# usage: test.sh
set -eu
cd "$(dirname "$0")"

# the swap files would warn about files open in another editor
nvim -n --headless -u NORC --noplugin -c "luafile test.lua"
