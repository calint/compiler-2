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
#   report  merge the profiles into 'report/'
#
# 'VERSIONS=first' or 'VERSIONS=last' compiles with that '--versions' and runs
# only the cases that run the program, see 'test-versions.sh'

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BIN="$SCRIPT_DIR/../../baz"
MACHINE="${MACHINE:-x86_64}"
OPTS="--vars=0x40000 --checks=upper,lower,line --reproduce-source"
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

# a plain serial console passes every input byte to the program
QEMU_SYSTEM=(qemu-system-riscv32 -machine virt -bios none -display none
    -serial stdio -monitor none -kernel gen-rv32i.bin)

# the image of the sector functions, see 'SDCARD'
FPGA_EMULATOR_DIR="$SCRIPT_DIR/../../fpga-emulator"
FPGA_EMULATOR=("$FPGA_EMULATOR_DIR/osqa" gen-rv32i.bin sdcard.img)

case "$ACTION" in
clean)
    echo $SEP
    # 'find' since the profiles outgrow the argument list of 'rm'
    find . -maxdepth 1 -name '*.profraw' -delete
    rm -f -- baz.profdata cov.err profiles.txt rv32i-driver gen gen.o gen.s diff.baz out err
    rm -f -- tests/gen tests/gen.o tests/gen.s tests/gen-nopt.s tests/gen-rv32i.bin tests/diff.baz tests/out tests/err
    rm -f -- tests/gen-image.o tests/gen-image.elf tests/gen-image.bin tests/sdcard.img
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
    rm -f -- profiles.txt
    objects=(-object="$BIN")
    # the driver of 'test-rv32i.sh' reaches lines the compiler cases do not
    if [[ -x rv32i-driver ]]; then
        objects+=(-object=rv32i-driver)
    fi
    # 'llvm-cov' warns about 4 functions of the merged profile of the compiler
    # and the driver that it cannot match, the functions are not identified
    llvm-cov show -format=html -output-dir=report/ -instr-profile=baz.profdata "${objects[@]}" 2>cov.err
    grep -v 'functions have mismatched data' cov.err >&2 || true
    rm -f -- cov.err
    # plain text table of the same data, small enough to commit and diff
    llvm-cov report -instr-profile=baz.profdata "${objects[@]}" >report/summary.txt 2>/dev/null
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

export LLVM_PROFILE_FILE="$SCRIPT_DIR/$MACHINE-%p.profraw"

if [[ $MACHINE == rv32i-fpga && ! -x ${FPGA_EMULATOR[0]} ]]; then
    "$FPGA_EMULATOR_DIR/make.sh"
fi

cd "$SCRIPT_DIR/tests"

rm -f gen.s out err

echo $SEP
echo "running: $BIN --target=$MACHINE $OPTS $VERSION_OPTS"
echo $SEP

export UBSAN_OPTIONS="print_stacktrace=1"
export ASAN_OPTIONS="fast_unwind_on_fatal=0:print_stacktrace=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1:external_symbolizer=1"
export ASAN_SYMBOLIZER_PATH="$(which llvm-symbolizer)"
export ASAN_SYMBOLIZE=1

# the built-in assembler must produce the bytes llvm assembles from 'gen.s'
verify-rv32i-image() {
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen-image.o
    ld.lld -m elf32lriscv -T "$SCRIPT_DIR/rv32i-image.ld" -e _start -o gen-image.elf gen-image.o
    llvm-objcopy -O binary gen-image.elf gen-image.bin
    if ! cmp gen-image.bin gen-rv32i.bin; then
        echo "FAILED. image differs from assembled 'gen.s'. see: llvm-objdump -d gen-image.elf"
        exit 1
    fi
}

# Common: compile and assemble
assemble_and_link() {
    case "$MACHINE" in
    x86_64)
        nasm -w+error -f elf64 gen.s
        ld -s -T "$SCRIPT_DIR/../../baz.ld" -o gen gen.o
        ;;
    rv32i)
        llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen.o
        ld.lld -m elf32lriscv -e _start -o gen gen.o
        verify-rv32i-image
        ;;
    # the compiler writes the image 'gen-rv32i.bin'
    rv32i-qemu | rv32i-fpga) verify-rv32i-image ;;
    esac
}

# the uart has no end of input, so ctrl-d ends it like at a terminal and a
# second one ends a last line without newline
run_uart() {
    local input=/dev/stdin
    if [[ -t 0 ]]; then
        input=/dev/null
    fi
    { cat "$input"; printf '\x04\x04'; } | "$@"
}

# sets 'PROGRAM_COMMAND' to the command that runs the program of the target
program_command() {
    case "$MACHINE" in
    x86_64) PROGRAM_COMMAND=(./gen) ;;
    rv32i) PROGRAM_COMMAND=(qemu-riscv32 ./gen) ;;
    rv32i-qemu) PROGRAM_COMMAND=("${QEMU_SYSTEM[@]}") ;;
    rv32i-fpga) PROGRAM_COMMAND=("${FPGA_EMULATOR[@]}") ;;
    esac
}

execute_program() {
    program_command
    case "$MACHINE" in
    rv32i-qemu | rv32i-fpga) run_uart "${PROGRAM_COMMAND[@]}" ;;
    *) "${PROGRAM_COMMAND[@]}" ;;
    esac
}

# the uart carries both output streams, so errors are in the output
execute_program_errors_to_out() {
    case "$MACHINE" in
    rv32i-qemu | rv32i-fpga) execute_program >out ;;
    *) execute_program 2>out ;;
    esac
}

compile_and_build() {
    if ! "$BIN" "$SRC.baz" --target="$MACHINE" --bin=gen-rv32i.bin $OPTS $VERSION_OPTS 2>err >gen.s; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

compile_and_build_no_checks() {
    if ! "$BIN" "$SRC.baz" --target="$MACHINE" --bin=gen-rv32i.bin --reproduce-source $VERSION_OPTS 2>err >gen.s; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

compile_and_build_with_opts() {
    local opts="$1"
    if ! "$BIN" "$SRC.baz" --target="$MACHINE" --bin=gen-rv32i.bin $opts --reproduce-source $VERSION_OPTS 2>err >gen.s; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

# the compile fails with exit code 1, its message is in 'out'
compile_expect_error() {
    set +e
    "$BIN" "$SRC.baz" --target="$MACHINE" --bin=gen-rv32i.bin $OPTS >gen.s 2>out
    local exit_code=$?
    set -e
    if [[ $exit_code -ne 1 ]]; then
        echo "FAILED. expected compiler exit 1 got $exit_code"
        exit 1
    fi
}

# Common: compare output with expected
check_output() {
    local expected="$1"
    # 'UPDATE=1' writes the output as the expected output, review the diff
    if [[ -n "${UPDATE:-}" ]]; then
        cp out "$expected"
        echo updated
    elif cmp -s out "$expected"; then
        echo ok
    else
        echo "FAILED. output differs. see: diff out $expected"
        exit 1
    fi
}

# the expected output of a case, 'NNN.TARGET.out' when the target differs, e.g.
# the fpga prints no panic message
expected_output_file() {
    if [[ -e "${SRC%.*}.$MACHINE.out" ]]; then
        echo "${SRC%.*}.$MACHINE.out"
    else
        echo "${SRC%.*}.out"
    fi
}

# the expected exit code of a case, the fpga has a code per failure in
# 'NNN.rv32i-fpga.exit' because it prints no message
expected_exit_code() {
    if [[ -e "${SRC%.*}.$MACHINE.exit" ]]; then
        cat "${SRC%.*}.$MACHINE.exit"
    else
        echo "$EXP"
    fi
}

# the sd card image 'sdcard.img' of the cases that follow, a sparse file of the
# size of the argument, e.g. 5G, or no image for 'none'
SDCARD() {
    rm -f sdcard.img
    if [[ $1 != none ]]; then
        truncate -s "$1" sdcard.img
    fi
}

# Test with exit code
RUN() {
    echo -n "$SRC: "
    compile_and_build

    set +e
    execute_program 2>err
    local exit_code=$?
    set -e

    if [ $exit_code -eq $EXP ]; then
        echo ok
    else
        echo "FAILED. expected $EXP got $exit_code"
        exit 1
    fi
}

RUN_ERR() {
    echo -n "$SRC: "
    compile_and_build

    set +e
    execute_program_errors_to_out
    local exit_code=$?
    set -e

    if [ $exit_code -ne $(expected_exit_code) ]; then
        echo "FAILED. expected $(expected_exit_code) got $exit_code"
        exit 1
    fi

    check_output "$(expected_output_file)"
}

RUN_ERR_OPTS() {
    echo -n "$SRC: "
    compile_and_build_with_opts "$1"

    set +e
    execute_program_errors_to_out
    local exit_code=$?
    set -e

    if [ $exit_code -ne $(expected_exit_code) ]; then
        echo "FAILED. expected $(expected_exit_code) got $exit_code"
        exit 1
    fi

    check_output "$(expected_output_file)"
}

RUN_NO_CHECKS() {
    echo -n "$SRC: "
    compile_and_build_no_checks

    set +e
    execute_program 2>err
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
    execute_program >out
    check_output "$(expected_output_file)"
}

# Test with input file
DIFFINP() {
    echo -n "$SRC: "
    compile_and_build
    execute_program <"${SRC%.*}.in" >out
    check_output "$(expected_output_file)"
}

# Test with line-by-line input
DIFFINP2() {
    echo -n "$SRC: "
    compile_and_build

    program_command
    "$SCRIPT_DIR/input-lines.py" "${SRC%.*}.in" "${PROGRAM_COMMAND[@]}" >out 2>err

    check_output "$(expected_output_file)"
}

# Test with Python script
DIFFPY() {
    echo -n "$SRC: "
    compile_and_build
    MACHINE="$MACHINE" "./${SRC%.*}.py" >out
    check_output "$(expected_output_file)"
}

# Test with exit code and the jump optimizations as a diff from --nopt
DIFFNOPT() {
    echo -n "$SRC: "
    # the expected difference is that of the default version
    if [[ -n "$VERSION_OPTS" ]]; then
        echo skipped
        return
    fi
    if ! "$BIN" "$SRC.baz" --target="$MACHINE" --bin=gen-rv32i.bin $OPTS --nopt 2>err >gen-nopt.s; then
        echo "compiler failed. see 'err' and 'gen-nopt.s'" >&2
        exit 1
    fi
    compile_and_build

    set +e
    execute_program 2>err
    local exit_code=$?
    diff gen-nopt.s gen.s >out
    set -e

    if [ $exit_code -ne $(expected_exit_code) ]; then
        echo "FAILED. expected $(expected_exit_code) got $exit_code"
        exit 1
    fi

    check_output "${SRC%.*}.$MACHINE.diff"
}

# Test with output comparison of compiler (no input)
COMPERR() {
    echo -n "$SRC: "
    # the program is not run
    if [[ -n "$VERSION_OPTS" ]]; then
        echo skipped
        return
    fi
    compile_expect_error
    local expected="${SRC%.*}.$COMPILER_MACHINE.out"
    if [[ ! -f "$expected" ]]; then
        expected="${SRC%.*}.out"
    fi
    check_output "$expected"
}

# the emulator opens the image also for the cases that do not use it
SDCARD 0

# Run all test cases
source "$SCRIPT_DIR/cases.sh"

# Cleanup
rm -f gen gen.o gen.s gen-nopt.s gen-rv32i.bin diff.baz out err sdcard.img
rm -f gen-image.o gen-image.elf gen-image.bin

echo $SEP
