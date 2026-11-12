#!/bin/sh
# profiles one compile with 'perf': builds 'build/baz' with cmake
# ('CMakeLists.txt', used only here), records it and writes the symbols by
# overhead to 'perf-report.txt'; 'qa/perf/perf.sh' profiles the whole test
# suite
# usage: scripts/perf.sh [options of baz] [NAME.baz]
set -eu
cd "$(dirname "$0")"
# the build directory is not tracked
mkdir -p ../build
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
