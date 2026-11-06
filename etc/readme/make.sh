#!/bin/sh
# writes the README.md of the repository from the parts in this directory, the
# usage of 'baz', the line counts of the source and the listings of 'prog.baz'
# note: the README is written to a temporary file then moved, so a failure
#       never leaves a truncated one
set -eu
cd "$(dirname "$0")"

if ! command -v cloc >/dev/null; then
    echo "error: 'cloc' is needed to count the lines of the source" >&2
    exit 1
fi
if [ ! -x ../../baz ]; then
    echo "error: '../../baz' is missing, build it with '../../make.sh build'" >&2
    exit 1
fi

OUT="$(mktemp ../../README.md.XXXXXX)"
trap 'rm -f "$OUT"' EXIT

cat README.1.md >"$OUT"
# run from the root so the usage shows the path used in the examples
(cd ../.. && ./baz --help) >>"$OUT"
cat README.2.md >>"$OUT"
cloc ../../src/ | sed '1,2d' >>"$OUT"
cat README.3.md >>"$OUT"
cat ../../prog.baz >>"$OUT"
cat README.4.md >>"$OUT"
cat ../../prog-without-comments.s >>"$OUT"
cat README.5.md >>"$OUT"
cat ../../prog.s >>"$OUT"
cat README.6.md >>"$OUT"
# note: 'mktemp' creates the file readable only by the owner
chmod 644 "$OUT"
mv "$OUT" ../../README.md
