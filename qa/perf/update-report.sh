#!/bin/sh
set -e

cd "$(dirname "$0")"

# Usage:
#   ./update-report.sh           # run coverage tests with nobuild
#   ./update-report.sh build     # rebuild via run-tests.sh and then profile
# Env:
#   PERF_FREQ=400 (sampling frequency)
TEST_MODE="${1:-nobuild}"
PERF_FREQ="${PERF_FREQ:-400}"

echo "[perf] recording full suite (mode: $TEST_MODE, freq: $PERF_FREQ)"
perf record -F "$PERF_FREQ" --call-graph dwarf -o all.data -- ../coverage/run-tests.sh "$TEST_MODE"

echo "[perf] writing full report"
perf report -i all.data --stdio-color never --stdio --no-children --sort overhead,comm,dso,symbol --percent-limit 0.10 > report-all.txt

echo "[perf] writing compiler-only flat report"
perf report -i all.data --stdio-color never --stdio --call-graph none --no-children --comms baz --sort overhead,symbol --percent-limit 0.01 > baz-flat.txt

echo "[perf] extracting top compiler symbols"
python - <<'PY'
import re
from pathlib import Path

report = Path("baz-flat.txt")
out = Path("top-baz.txt")

rows = []
for ln in report.read_text(errors="ignore").splitlines():
    m = re.match(r"\s*([0-9]+\.[0-9]+)%\s+\[\.\]\s+(.+?)\s+-\s+-\s*$", ln)
    if m:
        rows.append((float(m.group(1)), m.group(2).strip()))

rows.sort(reverse=True)
pat = re.compile(r"\b(toc|stmt_[a-z0-9_]*|expr_[a-z0-9_]*|jump_optimizer|program|tokenizer|x86|utils|type|statement)::")
project = [(p, s) for p, s in rows if pat.search(s)]

lines = ["Top compiler symbols (flat, no-children):"]
lines.extend(f"{p:6.2f}%  {s}" for p, s in project[:80])
out.write_text("\n".join(lines) + "\n")
print("\n".join(lines[:21]))
PY

echo "[perf] done"
echo "  data:   all.data"
echo "  all:    report-all.txt"
echo "  flat:   baz-flat.txt"
echo "  top:    top-baz.txt"
echo "[perf] quick guide"
echo "  top-baz.txt      : baz-only hotspots ranked by %"
echo "  baz-flat.txt     : full flat list for baz symbols (drill-down)"
echo "  report-all.txt   : whole-process view incl. stdlib/system costs"
echo "  all.data         : raw samples for custom perf report queries"
