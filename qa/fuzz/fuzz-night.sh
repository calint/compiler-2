#!/bin/sh
# runs all fuzzers again and again with new seeds until a stop time, then
# lists the findings that are new
# usage: fuzz-night.sh [hours] [first seed]
# 'hours' defaults to 8, a round starts only before the stop time and takes
# 15 to 45 minutes. the log is 'findings/night.log'
set -eu
cd "$(dirname "$0")"

hours=${1:-8}
seed=${2:-$(date +%s)}
stop=$(($(date +%s) + hours * 3600))

mkdir -p findings
log=findings/night.log
stamp=findings/.night-start
: > "$stamp"

echo "night run: $hours hours from $(date), first seed $seed" | tee -a "$log"
round=0
while [ "$(date +%s)" -lt "$stop" ]; do
    scale=$((1 + round % 3))
    echo "=== round $round scale $scale seed $((seed + round)) $(date)" | tee -a "$log"
    ./fuzz-all.sh "$scale" $((seed + round)) >> "$log" 2>&1 || true
    round=$((round + 1))
done

echo "=== done after $round rounds, $(date)" | tee -a "$log"
echo "=== new findings" | tee -a "$log"
find findings -name '*.txt' -newer "$stamp" ! -name night.log | sort | while read -r f; do
    echo "$f: $(sed -n 2p "$f" | cut -c1-120)" | tee -a "$log"
done
