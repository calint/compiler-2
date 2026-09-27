#!/bin/sh
set -e
cd $(dirname "$0")

cat README.1.md >README.md
# run from the root so the usage shows the path used in the examples
(cd ../.. && ./baz --help) >>README.md
cat README.2.md >>README.md
cloc ../../src/ | sed '1,2d' >>README.md
cat README.3.md >>README.md
cat ../../prog.baz >>README.md
cat README.4.md >>README.md
cat ../../prog-without-comments.s >>README.md
cat README.5.md >>README.md
cat ../../prog.s >>README.md
cat README.6.md >>README.md
cp README.md ../..
