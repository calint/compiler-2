#!/bin/sh
set -e
cd $(dirname "$0")
cd ..
cd build
rm -rf *
cmake ..
make
cd ..
SEP="--------------------------------------------------------------------------------"
CMD="sudo perf record -g build/baz $@"
echo $SEP
echo $CMD
echo $SEP
$CMD >/dev/null
sudo chmod +r perf.data
echo
perf report --stdio --no-children --sort symbol,overhead |
  grep -E "^\s+[0-9]+\.[0-9]+%\s+\[.*\]" |
  sed 's/\s\+-\s\+-.*$//' |
  grep -v "\[k\]" |
  sort -t'%' -k1 -rn |
  tee perf-report.txt

echo $SEP
