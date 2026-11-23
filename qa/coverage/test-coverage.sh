#!/bin/bash
# runs the compiler cases of 'cases.sh' for one target and builds the coverage
# report from the profiles of all runs
#
# usage: test-coverage.sh [--target=x86|x86_64|rv32i|rv32i-qemu|rv32i-fpga]
#                         clean|build|run|report
#   clean   remove profiles, reports and generated files
#   build   build the compiler with coverage and sanitizers
#   run     run all cases for the target; x86_64 runs the program natively,
#           rv32i under qemu user mode, rv32i-qemu in a qemu system with a
#           uart, and rv32i-fpga in the fpga emulator
#   report  merge the profiles into 'report/' and remove them
#
# 'UPDATE=1' writes the output of each case as the expected file it would be
# compared with ('.out', '.diff' or the kind of a listing); for a new
# 'NNN.TARGET.out' create the file first, else 'NNN.out' is written
# 'ONLY=NNNN' runs only the case 'SRC=NNNN', e.g. 'ONLY=0042'
# 'VERSIONS=first', 'last' or 'nested' compiles with that '--versions' and runs
# only the cases that run the program, except the '--nopt' diffs, see
# 'test-versions.sh'

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BIN="$SCRIPT_DIR/../../baz"
MACHINE=x86_64
# the options of a case without its own, also in 'compare-listings.sh'
DEFAULT_OPTS="--vars=0x40000 --checks=upper,lower,line"
# also with the options of a case: every source is reproduced exactly
REPRODUCE="--reproduce-source"
# the version of the same code that is kept, empty for the default
VERSION_OPTS="${VERSIONS:+--versions=$VERSIONS}"
SEP="--------------------------------------------------------------------------------"

usage() {
    echo "usage: $0 [--target=x86|x86_64|rv32i|rv32i-qemu|rv32i-fpga] clean|build|run|report"
}

ACTION=
for arg in "$@"; do
    case "$arg" in
    --target=*) MACHINE="${arg#--target=}" ;;
    clean | build | run | report)
        if [[ -n "$ACTION" ]]; then
            usage >&2
            exit 1
        fi
        ACTION="$arg"
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 1
        ;;
    esac
done

case "$MACHINE" in
x86) MACHINE=x86_64 ;;
x86_64 | rv32i | rv32i-qemu | rv32i-fpga) ;;
*)
    echo "unsupported target: $MACHINE" >&2
    exit 1
    ;;
esac

# the uart targets compile like rv32i, so they share the rv32i compiler messages
COMPILER_MACHINE="${MACHINE%-qemu}"
COMPILER_MACHINE="${COMPILER_MACHINE%-fpga}"

# the rv32i targets write an image, x86_64 takes no '--bin'
IMAGE_OPTS=()
if [[ $MACHINE != x86_64 ]]; then
    IMAGE_OPTS=(--bin=gen-rv32i.bin)
fi

# a plain serial console passes every input byte to the program
QEMU_SYSTEM=(qemu-system-riscv32 -machine virt -bios none -display none
    -serial stdio -monitor none -kernel gen-rv32i.bin)

# the emulator runs the image with the sd card image of the case, see
# 'make_sdcard'
FPGA_EMULATOR_DIR="$SCRIPT_DIR/../../fpga-emulator"
FPGA_EMULATOR=("$FPGA_EMULATOR_DIR/osqa" gen-rv32i.bin sdcard.img)

case "$ACTION" in
clean)
    echo $SEP
    # 'find' since the profiles and images outgrow the argument list of 'rm'
    find . tests -maxdepth 1 -name '*.profraw' -delete
    find tests -maxdepth 1 \( -name '*-rv32i*.bin' -o -name '*.core' \) -delete
    # the program, object and listing of a case that 'run.sh' compiled
    find tests -maxdepth 1 -regextype posix-extended -regex 'tests/[0-9]{4}(\.o|\.s)?' -delete
    rm -f -- baz.profdata baz-*.profdata profiles.txt
    rm -f -- tests/gen tests/gen.o tests/gen.s tests/gen-nopt.s tests/diff.baz tests/out tests/err
    rm -f -- tests/gen-image.elf tests/gen-image.bin tests/sdcard.img
    rm -rf -- report/
    echo removed reports
    echo $SEP
    exit 0
    ;;
build)
    echo "$SEP"
    ../../make.sh build prof asan
    echo "$SEP"
    exit 0
    ;;
report)
    # a file list since the profiles outgrow the argument list; each line is
    # 'weight,file', the explicit weight keeps a comma in a name, e.g. the
    # profile of 'cli---checks=noub,-division'
    find . -maxdepth 1 -name '*.profraw' | sed 's/^/1,/' >profiles.txt
    if [[ ! -s profiles.txt ]]; then
        rm -f -- profiles.txt
        echo "no coverage profiles found; build with coverage and run tests first" >&2
        exit 1
    fi
    llvm-profdata merge -o baz.profdata -sparse -input-files=profiles.txt
    find . -maxdepth 1 -name '*.profraw' -delete
    rm -f -- profiles.txt
    llvm-cov show -format=html -output-dir=report/ -instr-profile=baz.profdata "$BIN"
    # plain text table of the same data
    llvm-cov report -instr-profile=baz.profdata "$BIN" >report/summary.txt 2>/dev/null
    echo $SEP
    echo "coverage report generated in $SCRIPT_DIR/report/"
    echo $SEP
    exit 0
    ;;
run) ;;
*)
    usage >&2
    exit 1
    ;;
esac

# the expected outputs are those of the default version
if [[ -n "$VERSION_OPTS" && -n "${UPDATE:-}" ]]; then
    echo "UPDATE is not supported with VERSIONS" >&2
    exit 1
fi

# the runs merge their counts into a pool of 8 profiles per build instead of
# writing one each, the merged report is the same
export LLVM_PROFILE_FILE="$SCRIPT_DIR/$MACHINE-%8m.profraw"

# the emulator is rebuilt when missing or older than a source
if [[ $MACHINE == rv32i-fpga ]]; then
    if [[ ! -x ${FPGA_EMULATOR[0]} || -n "$(find "$FPGA_EMULATOR_DIR/src" -newer "${FPGA_EMULATOR[0]}" -print -quit)" ]]; then
        "$FPGA_EMULATOR_DIR/make.sh"
    fi
fi

cd "$SCRIPT_DIR/tests"

# a program gets input only from the '.in' file of its case, not from the
# terminal, pipe or socket the script was started with
exec </dev/null

rm -f gen.s out err
# a crash under qemu would write a core file 'qemu_gen_*.core'
ulimit -c 0

echo $SEP
echo "running: $BIN --target=$MACHINE $DEFAULT_OPTS $REPRODUCE $VERSION_OPTS"
echo $SEP

export UBSAN_OPTIONS="print_stacktrace=1"
export ASAN_OPTIONS="fast_unwind_on_fatal=0:print_stacktrace=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1:external_symbolizer=1"
export ASAN_SYMBOLIZER_PATH="$(which llvm-symbolizer)"
export ASAN_SYMBOLIZE=1

# the built-in assembler must produce the bytes llvm assembles from 'gen.s';
# leaves the object 'gen.o' that the rv32i target links
verify_rv32i_image() {
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen.o
    ld.lld -m elf32lriscv -T "$SCRIPT_DIR/rv32i-image.ld" -e _start -o gen-image.elf gen.o
    llvm-objcopy -O binary gen-image.elf gen-image.bin
    if ! cmp gen-image.bin gen-rv32i.bin; then
        echo "FAILED. image differs from assembled 'qa/coverage/tests/gen.s'. see: llvm-objdump -d qa/coverage/tests/gen-image.elf"
        exit 1
    fi
}

# links 'gen' from 'gen.s' where the target runs it, and checks the image of
# the rv32i targets
assemble_and_link() {
    case "$MACHINE" in
    x86_64)
        nasm -w+error -f elf64 gen.s
        ld -s -T "$SCRIPT_DIR/../../baz.ld" -o gen gen.o
        ;;
    rv32i)
        verify_rv32i_image
        ld.lld -m elf32lriscv -e _start -o gen gen.o
        ;;
    # the compiler writes the image 'gen-rv32i.bin'
    rv32i-qemu | rv32i-fpga) verify_rv32i_image ;;
    esac
}

# the uart has no end of input, so ctrl-d ends it like at a terminal and a
# second one ends a last line without newline
run_uart() {
    {
        cat
        printf '\x04\x04'
    } | "$@"
}

# a program that runs longer is stopped and exits with 124; '--foreground'
# keeps it in the process group of the script, so ctrl-c stops it too
# note: 10 s is about 30 times the slowest case, 0.3 s under qemu user mode
PROGRAM_TIMEOUT=(timeout --foreground 10)

# a compile that runs longer, e.g. of a compiler that loops, is stopped the
# same way and fails its case
# note: 60 s is about 500 times the slowest compile of an optimized build
COMPILE_TIMEOUT=(timeout --foreground 60)

# sets 'PROGRAM_COMMAND' to the command that runs the program of the target
program_command() {
    case "$MACHINE" in
    x86_64) PROGRAM_COMMAND=(./gen) ;;
    rv32i) PROGRAM_COMMAND=(qemu-riscv32 ./gen) ;;
    rv32i-qemu) PROGRAM_COMMAND=("${QEMU_SYSTEM[@]}") ;;
    rv32i-fpga) PROGRAM_COMMAND=("${FPGA_EMULATOR[@]}") ;;
    esac
    PROGRAM_COMMAND=("${PROGRAM_TIMEOUT[@]}" "${PROGRAM_COMMAND[@]}")
}

execute_program() {
    program_command
    case "$MACHINE" in
    rv32i-qemu | rv32i-fpga) run_uart "${PROGRAM_COMMAND[@]}" ;;
    *) "${PROGRAM_COMMAND[@]}" ;;
    esac
}

# the errors of the program to 'out'; the uart carries both output streams, so
# on the uart targets its output too, else the output goes to the log
execute_program_errors_to_out() {
    case "$MACHINE" in
    rv32i-qemu | rv32i-fpga) execute_program >out ;;
    *) execute_program 2>out ;;
    esac
}

# compiles the case for the target '$2' with the other arguments to the listing
# '$1', the messages to 'err'; a failed compile fails the case
compile_case() {
    local listing=$1
    local target=$2
    shift 2
    if ! "${COMPILE_TIMEOUT[@]}" "$BIN" "$SRC.baz" --target="$target" "${IMAGE_OPTS[@]}" "$@" 2>err >"$listing"; then
        echo "compiler failed. see 'qa/coverage/tests/err' and 'qa/coverage/tests/$listing'" >&2
        exit 1
    fi
}

# also makes the sd card image of the case, see 'make_sdcard'
compile_and_build() {
    # shellcheck disable=SC2086 # the options are words
    compile_case gen.s "$MACHINE" $CASE_OPTS $REPRODUCE $VERSION_OPTS
    assemble_and_link
    make_sdcard
}

# the compile fails with exit code 1, its message is in 'out'
compile_expect_error() {
    local exit_code=0
    # shellcheck disable=SC2086 # the options are words
    "${COMPILE_TIMEOUT[@]}" "$BIN" "$SRC.baz" --target="$MACHINE" "${IMAGE_OPTS[@]}" $CASE_OPTS $REPRODUCE >gen.s 2>out || exit_code=$?
    if [[ $exit_code -ne 1 ]]; then
        echo "FAILED. expected compiler exit 1 got $exit_code"
        exit 1
    fi
}

# 'UPDATE=1' writes the output as the expected output '$1', review the diff
check_output() {
    local expected="$1"
    if [[ -n "${UPDATE:-}" ]]; then
        cp out "$expected"
        echo updated
    elif cmp -s out "$expected"; then
        echo ok
    else
        echo "FAILED. output differs. see: diff qa/coverage/tests/out qa/coverage/tests/$expected"
        exit 1
    fi
}

# prints the first of the files that exists, else the last
first_existing() {
    local file
    for file in "$@"; do
        if [[ -e "$file" ]]; then
            echo "$file"
            return
        fi
    done
    echo "$file"
}

# the expected output of a run: 'NNN.TARGET.out' for the target of the run,
# else for the target of the compiler, e.g. 'NNN.rv32i.out' for rv32i-fpga,
# else 'NNN.out'
expected_run_output() {
    first_existing "$SRC.$MACHINE.out" "$SRC.$COMPILER_MACHINE.out" "$SRC.out"
}

# the expected output of a compile: 'NNN.TARGET.out' for the target of the
# compiler, else 'NNN.out'
expected_compile_output() {
    first_existing "$SRC.$COMPILER_MACHINE.out" "$SRC.out"
}

# fails the case when the exit code '$2' is not the expected '$1' or the case
# sets no expected code
expect_exit() {
    local expected=$1
    local exit_code=$2
    if [[ -z $expected ]]; then
        echo "FAILED. the case sets no 'EXP'"
        exit 1
    fi
    if [[ $exit_code -ne $expected ]]; then
        echo "FAILED. expected $expected got $exit_code"
        exit 1
    fi
}

# true after printing 'skipped' when a version other than the default is
# compiled ('VERSIONS'), for a case that expects what the default compiles
skipped_with_versions() {
    if [[ -z "$VERSION_OPTS" ]]; then
        return 1
    fi
    echo skipped
    SKIPPED_COUNT=$((SKIPPED_COUNT + 1))
}

# the sd card image 'sdcard.img' of the case, a sparse file of the size of its
# 'SDCARD', e.g. 'SDCARD=5G', empty without it; the emulator opens the image
# also for the cases that do not use it
make_sdcard() {
    rm -f sdcard.img
    truncate -s "$CASE_SDCARD" sdcard.img
}

# prints the name of the case and moves its variables 'EXP', 'OPTS' and
# 'SDCARD' to 'CASE_EXP', 'CASE_OPTS' and 'CASE_SDCARD'; clearing them keeps
# one set with '&&' instead of as a prefix out of the next case
begin_case() {
    echo -n "$SRC: "
    CASE_COUNT=$((CASE_COUNT + 1))
    CASE_EXP=${EXP-}
    CASE_OPTS=${OPTS-$DEFAULT_OPTS}
    CASE_SDCARD=${SDCARD:-0}
    unset EXP OPTS SDCARD
}

# the helpers of the cases, see 'cases.sh'; a program judged by its output
# must exit 0, one judged by its exit code alone ('RUN', 'DIFFNOPT') writes
# its output to 'out' and 'err', not into the log

RUN() {
    begin_case
    compile_and_build
    local exit_code=0
    execute_program >out 2>err || exit_code=$?
    expect_exit "$CASE_EXP" $exit_code
    echo ok
}

RUN_ERR() {
    begin_case
    compile_and_build
    local exit_code=0
    execute_program_errors_to_out || exit_code=$?
    expect_exit "$CASE_EXP" $exit_code
    check_output "$(expected_run_output)"
}

DIFF() {
    begin_case
    compile_and_build
    local exit_code=0
    execute_program >out || exit_code=$?
    expect_exit 0 $exit_code
    check_output "$(expected_run_output)"
}

DIFFINP() {
    begin_case
    compile_and_build
    local exit_code=0
    execute_program <"$SRC.in" >out || exit_code=$?
    expect_exit 0 $exit_code
    check_output "$(expected_run_output)"
}

DIFFLINES() {
    begin_case
    compile_and_build
    program_command
    local exit_code=0
    "$SCRIPT_DIR/input-lines.py" "$SRC.in" "${PROGRAM_COMMAND[@]}" >out 2>err || exit_code=$?
    expect_exit 0 $exit_code
    check_output "$(expected_run_output)"
}

# the script of the case runs the program with the command in
# 'PROGRAM_COMMAND_LINES', one argument per line, and reads the target of the
# run from 'MACHINE'
DIFFPY() {
    begin_case
    compile_and_build
    program_command
    local exit_code=0
    MACHINE="$MACHINE" PROGRAM_COMMAND_LINES="$(printf '%s\n' "${PROGRAM_COMMAND[@]}")" \
        "./$SRC.py" >out || exit_code=$?
    expect_exit 0 $exit_code
    check_output "$(expected_run_output)"
}

DIFFNOPT() {
    begin_case
    if skipped_with_versions; then
        return
    fi
    # shellcheck disable=SC2086 # the options are words
    compile_case gen-nopt.s "$MACHINE" $CASE_OPTS --nopt
    compile_and_build
    local exit_code=0
    execute_program >out 2>err || exit_code=$?
    expect_exit "$CASE_EXP" $exit_code
    # 'diff' exits 1 when the listings differ, the expected case
    diff gen-nopt.s gen.s >out || true
    check_output "$SRC.$MACHINE.diff"
}

# the part of the listing that 'listing.py' extracts for the kind of the
# argument, compiled for the target of the compiler with its defaults (no
# checks), compared with 'NNN.TARGET.KIND'; the 'OPTS' of the case are ignored
LISTING() {
    # the listing is compiled without options, an 'OPTS' prefix would be
    # ignored; read before 'begin_case' clears it
    if [[ -n ${OPTS+set} ]]; then
        echo "FAILED. case $SRC: 'LISTING' takes no 'OPTS'"
        exit 1
    fi
    begin_case
    local expected="$SRC.$COMPILER_MACHINE.$1"
    if skipped_with_versions; then
        return
    fi
    # 'UPDATE=1' writes the first one
    if [[ ! -e "$expected" && -z "${UPDATE:-}" ]]; then
        echo "FAILED. no expected listing 'qa/coverage/tests/$expected'"
        exit 1
    fi
    compile_case gen.s "$COMPILER_MACHINE"
    if ! "$SCRIPT_DIR/listing.py" "$1" "$SRC.baz" "$COMPILER_MACHINE" <gen.s >out; then
        echo "FAILED. see: qa/coverage/tests/out"
        exit 1
    fi
    check_output "$expected"
}

# the program is not run, so the versions do not matter
COMPERR() {
    begin_case
    if skipped_with_versions; then
        return
    fi
    compile_expect_error
    check_output "$(expected_compile_output)"
}

# every line of 'cases.sh' is blank, a comment, a case or a guard, so a
# malformed one fails before the cases run instead of passing silently, e.g.
# a case without its helper, or failing after those before it passed
check_cases_lines() {
    # one case: its source, the expectations of the helper and its options
    # note: 'LISTING' compiles with the defaults, so it takes no 'OPTS'
    local case_line='SRC=[0-9]{4} && ((EXP=[0-9]+ && )?(OPTS="[^"]*" )?(SDCARD=[0-9]+[KMGT]? )?(RUN|RUN_ERR|DIFF|DIFFINP|DIFFLINES|DIFFPY|DIFFNOPT|COMPERR)|LISTING [a-z]+)'
    # a guard on the targets that run the cases after it
    local guard_line='if \[\[ \$MACHINE [!=]= (x86_64|rv32i|rv32i-qemu|rv32i-fpga)( \|\| \$MACHINE == (rv32i|rv32i-fpga))? \]\]; then'
    local line_numbers
    line_numbers=$(sed -E "s/$case_line/CASE/g" "$SCRIPT_DIR/cases.sh" |
        grep -nvE "^ *(#.*)?$|^ *CASE$|^$guard_line( CASE;( else CASE;)? fi)?$|^(else|fi)$" |
        cut -d: -f1)
    if [[ -z $line_numbers ]]; then
        return
    fi
    local line_number
    for line_number in $line_numbers; do
        echo "cases.sh:$line_number: $(sed -n "${line_number}p" "$SCRIPT_DIR/cases.sh")"
    done
    echo "FAILED. malformed lines in 'cases.sh'"
    exit 1
}

check_cases_lines

# a failure also names the target, else only in the header far above
trap '[[ $? -eq 0 ]] || echo "FAILED. case $SRC on target $MACHINE"' EXIT

CASE_COUNT=0
SKIPPED_COUNT=0
if [[ -n ${ONLY:-} ]]; then
    # the lines of one case, also under a guard of the target
    source <(grep -E "SRC=$ONLY( |$)" "$SCRIPT_DIR/cases.sh")
    if [[ $CASE_COUNT -eq 0 ]]; then
        echo "FAILED. no case 'SRC=$ONLY' in 'cases.sh' for $MACHINE"
        exit 1
    fi
else
    source "$SCRIPT_DIR/cases.sh"
fi

rm -f gen gen.o gen.s gen-nopt.s gen-rv32i.bin diff.baz out err sdcard.img
rm -f gen-image.elf gen-image.bin

# a run that ends early has no such line
echo $SEP
echo "passed: $((CASE_COUNT - SKIPPED_COUNT)) cases, skipped: $SKIPPED_COUNT, target: $MACHINE${VERSION_OPTS:+ $VERSION_OPTS}"
echo $SEP
