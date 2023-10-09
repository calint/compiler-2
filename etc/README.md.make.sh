#!/bin/bash
set -e

# change to directory of the script
cd $(dirname "$0")

cat README.md.1 > ../README.md
cat ../prog.baz >> ../README.md
cat README.md.2 >> ../README.md
cat ../gen.s >> ../README.md
cat README.md.3 >> ../README.md
