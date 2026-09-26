#!/bin/bash
# tools:
#     llvm-profdata: 22.1.8
#          llvm-cov: 22.1.8
#           genhtml: 2.5
#   llvm-symbolizer: 22.1.8
#              nasm: 3.02
#                ld: 2.47
#           llvm-mc: 22.1.8
#            ld.lld: 22.1.8
#      llvm-objcopy: 22.1.8
#      qemu-riscv32: 11.1.1
# qemu-system-riscv32: 11.1.1
#           python3: 3.14.7

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BIN="$SCRIPT_DIR/../../baz"
MACHINE="${MACHINE:-x86_64}"
OPTS="--vars=262144 --checks=upper,lower,line --reproduce-source"
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

# the program does not use the sd card, so an empty image suffices
FPGA_EMULATOR_DIR="$SCRIPT_DIR/../../fpga-emulator"
FPGA_EMULATOR=("$FPGA_EMULATOR_DIR/osqa" gen-rv32i.bin /dev/null)

case "$ACTION" in
clean)
    echo $SEP
    rm -f -- *.profraw baz.profdata gen gen.o gen.s diff.baz out err
    rm -f -- tests/gen tests/gen.o tests/gen.s tests/gen-nopt.s tests/gen-rv32i.bin tests/diff.baz tests/out tests/err
    rm -f -- tests/gen-image.o tests/gen-image.elf tests/gen-image.bin
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
    shopt -s nullglob
    profiles=(*.profraw)
    if [[ ${#profiles[@]} -eq 0 ]]; then
        echo "no coverage profiles found; build with coverage and run tests first" >&2
        exit 1
    fi
    llvm-profdata merge -o baz.profdata -sparse "${profiles[@]}"
    llvm-cov show -format=html -output-dir=report/ -instr-profile=baz.profdata -object="$BIN"
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

export LLVM_PROFILE_FILE="$SCRIPT_DIR/$MACHINE-%p.profraw"

if [[ $MACHINE == rv32i-fpga && ! -x ${FPGA_EMULATOR[0]} ]]; then
    "$FPGA_EMULATOR_DIR/make.sh"
fi

cd "$SCRIPT_DIR/tests"

rm -f gen.s out err

echo $SEP
echo "running: $BIN --target=$MACHINE $OPTS"
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
        nasm -f elf64 gen.s
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

execute_program() {
    case "$MACHINE" in
    x86_64) ./gen ;;
    rv32i) qemu-riscv32 ./gen ;;
    rv32i-qemu) run_uart "${QEMU_SYSTEM[@]}" ;;
    rv32i-fpga) run_uart "${FPGA_EMULATOR[@]}" ;;
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
    "$BIN" "$SRC.baz" --target="$MACHINE" $OPTS 2>err >gen.s
    if [ $? -ne 0 ]; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

compile_and_build_no_checks() {
    "$BIN" "$SRC.baz" --target="$MACHINE" --reproduce-source 2>err >gen.s
    if [ $? -ne 0 ]; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

compile_and_build_with_opts() {
    local opts="$1"
    "$BIN" "$SRC.baz" --target="$MACHINE" $opts --reproduce-source 2>err >gen.s
    if [ $? -ne 0 ]; then
        echo "compiler failed. see 'err' and 'gen.s'" >&2
        exit 1
    fi
    assemble_and_link
}

# Common: compile and assemble
compile_expect_error() {
    set +e
    "$BIN" "$SRC.baz" --target="$MACHINE" $OPTS >gen.s 2>out
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
    if cmp -s out "$expected"; then
        echo ok
    else
        echo "FAILED. output differs. see: diff out $expected"
        exit 1
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

    if [ $exit_code -ne $EXP ]; then
        echo "FAILED. expected $EXP got $exit_code"
        exit 1
    fi

    check_output "${SRC%.*}.out"
}

RUN_ERR_OPTS() {
    echo -n "$SRC: "
    compile_and_build_with_opts "$1"

    set +e
    execute_program_errors_to_out
    local exit_code=$?
    set -e

    if [ $exit_code -ne $EXP ]; then
        echo "FAILED. expected $EXP got $exit_code"
        exit 1
    fi

    check_output "${SRC%.*}.out"
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
    check_output "${SRC%.*}.out"
}

# Test with input file
DIFFINP() {
    echo -n "$SRC: "
    compile_and_build
    execute_program <"${SRC%.*}.in" >out
    check_output "${SRC%.*}.out"
}

# Test with line-by-line input
DIFFINP2() {
    echo -n "$SRC: "
    compile_and_build

    local command=(./gen)
    if [ "$MACHINE" = rv32i ]; then
        command=(qemu-riscv32 ./gen)
    fi
    if [ "$MACHINE" = rv32i-qemu ]; then
        command=("${QEMU_SYSTEM[@]}")
    fi
    if [ "$MACHINE" = rv32i-fpga ]; then
        command=("${FPGA_EMULATOR[@]}")
    fi

    "$SCRIPT_DIR/input-lines.py" "${SRC%.*}.in" "${command[@]}" >out 2>err

    check_output "${SRC%.*}.out"
}

# Test with Python script
DIFFPY() {
    echo -n "$SRC: "
    compile_and_build
    MACHINE="$MACHINE" "./${SRC%.*}.py" >out
    check_output "${SRC%.*}.out"
}

# Test with exit code and the jump optimizations as a diff from --nopt
DIFFNOPT() {
    echo -n "$SRC: "
    if ! "$BIN" "$SRC.baz" --target="$MACHINE" $OPTS --nopt 2>err >gen-nopt.s; then
        echo "compiler failed. see 'err' and 'gen-nopt.s'" >&2
        exit 1
    fi
    compile_and_build

    set +e
    execute_program 2>err
    local exit_code=$?
    diff gen-nopt.s gen.s >out
    set -e

    if [ $exit_code -ne $EXP ]; then
        echo "FAILED. expected $EXP got $exit_code"
        exit 1
    fi

    check_output "${SRC%.*}.$MACHINE.diff"
}

# Test with output comparison of compiler (no input)
COMPERR() {
    echo -n "$SRC: "
    compile_expect_error
    local expected="${SRC%.*}.$COMPILER_MACHINE.out"
    if [[ ! -f "$expected" ]]; then
        expected="${SRC%.*}.out"
    fi
    check_output "$expected"
}

# Run all test cases
source "$SCRIPT_DIR/cases.sh"

# Cleanup
rm -f gen gen.o gen.s gen-nopt.s gen-rv32i.bin diff.baz out err
rm -f gen-image.o gen-image.elf gen-image.bin

echo $SEP
