#!/bin/sh
set -eu
cd "$(dirname "$0")"
cd ../build
rm -rf ./*
cmake ..
make
cd ..
SEP="--------------------------------------------------------------------------------"
echo "$SEP"
echo "sudo perf record -g build/baz $*"
echo "$SEP"
sudo perf record -g build/baz "$@" >/dev/null
sudo chmod +r perf.data
echo
perf report --stdio --no-children --sort symbol,overhead |
  grep -E "^\s+[0-9]+\.[0-9]+%\s+\[.*\]" |
  sed 's/\s\+-\s\+-.*$//' |
  grep -v "\[k\]" |
  sort -t'%' -k1 -rn |
  tee perf-report.txt

echo "$SEP"
