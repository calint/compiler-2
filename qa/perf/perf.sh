#!/bin/bash
set -e

cd "$(dirname "$0")"

export LC_ALL=C
export PERF_PAGER=cat

usage() {
    cat <<'HELP'
Usage: ./perf.sh [nobuild|build|report] [full|compile-only] [--reproduce-source]
  nobuild         Record the existing compiler (default).
  build           Build with -O3 and debug symbols, then record.
  report          Regenerate all reports from the existing recording.
  full            Run the test suite with live output (default).
  compile-only    Compile every t*.baz; do not assemble/link/execute.
    --reproduce-source  Enable source reproduction and verification (compile-only only).

Environment: PERF_FREQ=400, PERF_REPEATS=1 (compile-only sweeps).

Examples:
  ./perf.sh build
  PERF_REPEATS=5 ./perf.sh build compile-only
    PERF_REPEATS=5 ./perf.sh nobuild compile-only --reproduce-source
  ./perf.sh report compile-only

Outputs are written alongside this script; compile-only uses a suffix.
Compiler reports use baz-relative percentages; inclusive rows overlap.
Inline-frame expansion is disabled to keep call-chain symbols reliable.
TSV files have headers, numeric percentages, and no display truncation.
Keep the recorded binary unchanged until reports have been generated.
HELP
}

invalid_usage() {
    printf '[perf] %s\n' "$1" >&2
    usage >&2
    exit 2
}

if [[ "${1:-}" == --compile-workload ]]; then
    shift
    cd ../coverage
    for ((repeat = 1; repeat <= PERF_REPEATS; ++repeat)); do
        for src in t*.baz; do
            status=0
            ../../baz "$src" --vars=262144 --checks=upper,lower,line "$@" \
                >/dev/null 2>../perf/compile-last-error.txt || status=$?
            printf '[perf] sweep %s: %s exit=%s\n' "$repeat" "$src" "$status"
            if ((status >= 128)); then
                cat ../perf/compile-last-error.txt >&2
                exit "$status"
            fi
        done
    done
    exit 0
fi

if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
    usage
    exit 0
fi

MODE="${1:-nobuild}"
SCOPE="${2:-full}"
REPRODUCE_OPTION="${3:-}"
PERF_FREQ="${PERF_FREQ:-400}"
export PERF_REPEATS="${PERF_REPEATS:-1}"

if [[ $# -gt 3 ]]; then
    invalid_usage 'Too many arguments.'
fi
if [[ "$MODE" != nobuild && "$MODE" != build && "$MODE" != report ]]; then
    invalid_usage 'Mode must be nobuild, build, or report.'
fi
if [[ "$SCOPE" != full && "$SCOPE" != compile-only ]]; then
    invalid_usage 'Scope must be full or compile-only.'
fi
if [[ $# == 3 && "$REPRODUCE_OPTION" != --reproduce-source ]]; then
    invalid_usage 'The only third argument is --reproduce-source.'
fi
if [[ "$REPRODUCE_OPTION" == --reproduce-source && "$SCOPE" != compile-only ]]; then
    invalid_usage '--reproduce-source requires compile-only.'
fi
if [[ ! "$PERF_FREQ" =~ ^[1-9][0-9]*$ ]]; then
    invalid_usage 'PERF_FREQ must be a positive integer.'
fi
if [[ ! "$PERF_REPEATS" =~ ^[1-9][0-9]*$ ]]; then
    invalid_usage 'PERF_REPEATS must be a positive integer.'
fi

command -v perf >/dev/null
command -v python3 >/dev/null
SUFFIX=""
if [[ "$SCOPE" == compile-only ]]; then
    SUFFIX="-compile-only"
fi
if [[ "$REPRODUCE_OPTION" == --reproduce-source ]]; then
    SUFFIX+="-reproduce-source"
fi
DATA_FILE="all${SUFFIX}.data"

if [[ "$MODE" != report ]]; then
    if [[ "$MODE" == build ]]; then
        ../../make.sh -O3 build
    fi
    if [[ ! -x ../../baz ]]; then
        printf '%s\n' '[perf] compiler missing; use build mode.' >&2
        exit 1
    fi
    printf '[perf] recording %s (mode=%s, frequency=%s)\n' "$SCOPE" "$MODE" "$PERF_FREQ"
    if [[ "$SCOPE" == compile-only ]]; then
        printf '%s\n' '[perf] Nonzero compiler exits can be intentional error fixtures; this is not a correctness test.'
        workload=(./perf.sh --compile-workload)
        if [[ "$REPRODUCE_OPTION" == --reproduce-source ]]; then
            workload+=(--reproduce-source)
        fi
    else
        workload=(../coverage/run-tests-coverage.sh nobuild)
    fi
    perf record -F "$PERF_FREQ" --call-graph dwarf -o "$DATA_FILE" -- "${workload[@]}"
fi

python3 - "$DATA_FILE" "$SUFFIX" <<'PY'
import csv
import re
import subprocess
import sys
from pathlib import Path

data_file, suffix = sys.argv[1:]
base = [
    "perf", "--no-pager", "report", "-i", data_file,
    "--stdio-color", "never", "--stdio", "--no-inline",
]
artifacts = []


def write_text(name, text):
    path = Path(f"{name}{suffix}.txt")
    path.write_text(text)
    artifacts.append(path)


def perf_output(options):
    return subprocess.check_output(base + options, text=True, errors="replace")


def write_table(name, headers, rows):
    path = Path(f"{name}{suffix}.tsv")
    with path.open("w", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(headers)
        writer.writerows(rows)
    artifacts.append(path)


def report(name, fields, headers, options, percent_columns=1):
    args = ["--percent-limit", "0", "--call-graph", "none", "--fields", fields] + options
    write_text(name, perf_output(args))
    raw = perf_output(args + ["--field-separator", "\t"])
    rows = []
    for line in raw.splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        columns = [column.strip() for column in line.split("\t")]
        if not re.fullmatch(r"[0-9]+(?:\.[0-9]+)?%?", columns[0]):
            continue
        if len(columns) != len(headers):
            raise SystemExit(f"Unexpected perf columns in {name}: {line!r}")
        for index in range(percent_columns):
            columns[index] = columns[index].removesuffix("%")
        rows.append(columns)
    if not rows:
        raise SystemExit(f"No samples parsed for {name}; check recording and perf output")
    write_table(name, headers, rows)
    return rows


write_text("recording", perf_output(["--header-only"]))
report("report-all", "overhead,comm,dso,symbol",
       ["self_pct", "command", "dso", "symbol"], ["--no-children", "--percentage", "absolute"])
report("processes", "overhead,comm", ["self_pct", "command"],
       ["--no-children", "--percentage", "absolute"])
compiler = ["--comms", "baz", "--percentage", "relative"]
flat = report("baz-flat", "overhead,dso,symbol", ["self_pct", "dso", "symbol"],
              compiler + ["--no-children"])
report("baz-children", "overhead_children,overhead,dso,symbol",
       ["inclusive_pct", "self_pct", "dso", "symbol"], compiler + ["--children"], 2)
write_text("baz-callgraph", perf_output(compiler + ["--children", "--percent-limit", "0.5",
                                                   "--call-graph", "graph,0.5,caller"]))

owner = re.compile(r"^(?:.*?\s)?(toc|frame|ident_path|token|tokenizer|type|statement|program|"
                   r"x86|utils|jump_optimizer|stmt_\w+|expr_\w+|lut<[^>]+>)::")
project = []
groups = {}
unresolved = 0.0
for percent, dso, symbol in flat:
    clean = re.sub(r"^\[.\]\s*", "", symbol)
    match = None if clean.startswith(("std::", "void std::", "auto std::")) else owner.match(clean)
    if match:
        project.append((percent, dso, symbol))
    group = match.group(1) if match else "other (stdlib/system/unattributed)"
    groups[group] = groups.get(group, 0.0) + float(percent)
    if re.match(r"^(?:0x)?[0-9a-f]+$", clean) or "[unknown]" in clean:
        unresolved += float(percent)
project.sort(key=lambda row: float(row[0]), reverse=True)
write_table("top-baz", ["self_pct", "dso", "symbol"], project)
write_text("top-baz", "Compiler-owned symbols: self % of baz samples, not suite time.\n" +
           "\n".join(f"{float(percent):6.2f}%  {symbol}" for percent, _, symbol in project) + "\n")
group_rows = [(f"{value:.2f}", name) for name, value in
              sorted(groups.items(), key=lambda item: item[1], reverse=True)]
write_table("top-modules", ["self_pct", "symbol_owner"], group_rows)
write_text("top-modules", "Heuristic symbol-owner grouping of flat self cost; not inclusive module cost.\n"
           "Inlined/template/library work may be unattributed. Sums use rounded percentages.\n" +
           "\n".join(f"{float(percent):6.2f}%  {name}" for percent, name in group_rows) + "\n")
write_text("summary", "Percentage guide:\n"
           "  report-all/processes: share of all recorded samples.\n"
           "  baz-*: share of samples filtered to command baz, including libraries/kernel.\n"
           "  inclusive_pct includes callees: rows overlap, never sum them.\n"
           "  top-baz/top-modules: heuristic ownership, not all compiler work.\n"
           "  baz-callgraph: readable call chains, threshold 0.5%.\n"
           "  Inline-frame expansion is disabled (--no-inline); inlined work is attributed to containing functions.\n"
           "  TSV tables: header row, tab-separated, percentages without percent signs.\n"
           "  recording: perf metadata including event and recording command.\n"
           f"Unresolved baz self cost (approximate): {unresolved:.2f}%\n"
           "Low sample counts and unresolved symbols weaken hotspot rankings.\n"
           "Use build mode for optimized measurements; nobuild uses whatever binary exists.\n"
           "Compile-only exit statuses are informational, not correctness validation.\n")
print("[perf] reports generated:")
for path in artifacts:
    print(f"  {path}")
print(f"[perf] Start with summary{suffix}.txt and baz-children{suffix}.tsv")
PY
