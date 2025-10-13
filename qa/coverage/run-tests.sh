#!/bin/sh
# tools:
#   llvm-profdata: ?
#        llvm-cov: 15.0.7
#         genhtml: 1.16
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ "$1" != "nomake" ]; then
  ../../make.sh build prof asan
fi

BIN="../../baz"
OPTS="--stack=65536 --checks=bounds,line"

SEP="--------------------------------------------------------------------------------"
echo $SEP
echo "running: $BIN $OPTS"
echo $SEP

export UBSAN_OPTIONS="print_stacktrace=1"
export ASAN_OPTIONS="fast_unwind_on_fatal=0:print_stacktrace=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1:external_symbolizer=1"
export ASAN_SYMBOLIZER_PATH="$(which llvm-symbolizer)"
export ASAN_SYMBOLIZE=1

# Common: compile and assemble
compile_and_build() {
  rm -f error.log
  LLVM_PROFILE_FILE="${SRC%.*}.profraw" $BIN "$SRC.baz" $OPTS 2>error.log >gen.s
  if [ $? -ne 0 ]; then
    echo "compiler failed. see 'error.log' and 'gen.s'" >&2
    exit 1
  fi
  nasm -f elf64 gen.s
  ld -s -o gen gen.o
}

# Common: compare output with expected
check_output() {
  local expected="$1"
  if cmp -s out "$expected"; then
    echo ok
  else
    echo "FAILED. output differs. see: diff $expected out"
    exit 1
  fi
}

# Test with exit code
RUN() {
  echo -n "$SRC: "
  compile_and_build

  set +e
  ./gen
  local exit_code=$?
  set -e

  if [ $exit_code -eq $EXP ]; then
    echo ok
  else
    echo "FAILED. expected $EXP got $exit_code"
    exit 1
  fi
}

# Test with output comparison (no input)
DIFF() {
  echo -n "$SRC: "
  compile_and_build
  ./gen >out
  check_output "${SRC%.*}.out"
}

# Test with input file
DIFFINP() {
  echo -n "$SRC: "
  compile_and_build
  ./gen <"${SRC%.*}.in" >out
  check_output "${SRC%.*}.out"
}

# Test with line-by-line input
DIFFINP2() {
  echo -n "$SRC: "
  compile_and_build

  while read -r line; do
    echo "$line"
    sleep 0.001 # Small wait for single line to be received by `read`
  done <"${SRC%.*}.in" | ./gen >out

  check_output "${SRC%.*}.out"
}

# Test with Python script
DIFFPY() {
  echo -n "$SRC: "
  compile_and_build
  "./${SRC%.*}.py" >out
  check_output "${SRC%.*}.out"
}

# Run all test cases
source "$SCRIPT_DIR/run-tests-cases.sh"

# Cleanup
rm -f gen gen.o gen.s diff.baz out error.log

echo $SEP

# Process coverage data
llvm-profdata merge -o baz.profdata -sparse $(ls *.profraw)
llvm-cov export --format=lcov --instr-profile baz.profdata --object $BIN >lcov.info

# Generate report
# note: ignore "inconsistent" because of problems with understanding lambdas
genhtml --ignore-errors inconsistent,unsupported --quiet lcov.info --output-directory report/

echo $SEP
echo "coverage report generated in $(realpath "report/")"
rm -f *.profraw baz.profdata lcov.info
echo $SEP
