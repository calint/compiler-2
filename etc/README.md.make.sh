#!/bin/bash
set -e

# change to directory of the script
cd $(dirname "$0")

cat README.1.md >../README.md
cat ../prog.baz >>../README.md
cat README.2.md >>../README.md
cat ../gen.s >>../README.md
cat README.3.md >>../README.md
