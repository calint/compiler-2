#!/bin/bash
set -e

cat README.md.1 > README.md
cat prog.baz >> README.md
cat README.md.2 >> README.md
cat gen.s >> README.md
cat README.md.3 >> README.md
