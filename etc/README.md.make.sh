#!/bin/sh
set -e
cd $(dirname "$0")

cat README.1.md >../README.md
cloc ../src/ | sed '1,2d' >>../README.md
cat README.2.md >>../README.md
cat ../prog.baz >>../README.md
cat README.3.md >>../README.md
cat ../gen.s >>../README.md
cat README.4.md >>../README.md
