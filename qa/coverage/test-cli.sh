#!/bin/bash
# checks the command line of the compiler: option values that are accepted or
# rejected (the exit code is the expectation), the target selection, the
# binary name, and the '--nopt' and '--reproduce-source' behavior
#
# usage: test-cli.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/tests"

BIN="$SCRIPT_DIR/../../baz"
export LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-%p.profraw"

CLI() {
    echo -n "cli $1: "
    set +e
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-$1.profraw" "$BIN" "$1" "${@:3}" >/dev/null 2>err
    local exit_code=$?
    set -e

    if [[ $exit_code -eq "$2" ]]; then
        echo ok
    else
        echo "FAILED. expected $2 got $exit_code"
        exit 1
    fi
}

CLI_REPRODUCE_SOURCE() {
    echo -n "cli --reproduce-source: "
    rm -f diff.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-default.profraw" "$BIN" 015.baz >gen.s 2>err
    [[ ! -e diff.baz ]]
    cp 001.baz diff.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-default-nopt.profraw" "$BIN" 015.baz --nopt >out 2>err
    cmp -s diff.baz 001.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-reproduce.profraw" "$BIN" 015.baz --reproduce-source >out 2>err
    cmp -s diff.baz 015.baz
    cmp -s gen.s out
    rm -f diff.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-reproduce-nopt.profraw" "$BIN" --reproduce-source 015.baz --nopt >out 2>err
    cmp -s diff.baz 015.baz
    echo ok
}

CLI_TARGETS() {
    echo -n "cli target selection: "
    "$BIN" 015.baz >gen.s 2>err
    "$BIN" --target=x86_64 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --nopt 015.baz >gen.s 2>err
    "$BIN" --target=x86_64 --nopt 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --target=rv32i 430.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fxq ".option norvc" gen.s
    grep -Eq '^[[:space:]]*ecall$' gen.s
    "$BIN" --target=rv32i --nopt 430.baz >out 2>err
    [[ ! -s err ]]
    [[ $(wc -l <gen.s) -le $(wc -l <out) ]]
    # without an operating system the uart routines replace system calls
    "$BIN" --target=rv32i-qemu --stack=4096 430.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -Ec '^[[:space:]]*ecall$' gen.s) -eq 0 ]]
    grep -Eq '^[[:space:]]*li t0, 4096$' gen.s
    rm -f 430-rv32i.bin 430-rv32i-qemu.bin
    echo ok
}

CLI_BINARY_NAME() {
    echo -n "cli binary name: "
    rm -f 430-rv32i-qemu.bin gen-rv32i.bin
    # the default is the source without extension followed by the target
    "$BIN" --target=rv32i-qemu 430.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ -s 430-rv32i-qemu.bin ]]
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin 430.baz >out 2>err
    [[ ! -s err ]]
    cmp -s 430-rv32i-qemu.bin gen-rv32i.bin
    cmp -s gen.s out
    rm -f 430-rv32i-qemu.bin gen-rv32i.bin
    # x86_64 writes no binary
    "$BIN" --bin=gen-rv32i.bin 430.baz >gen.s 2>err
    [[ ! -e gen-rv32i.bin ]]
    echo ok
}

CLI_JUMP_OPTIMIZATIONS() {
    echo -n "cli x86 jump optimizations: "
    "$BIN" --target=x86_64 --nopt 441.baz >gen.s 2>err
    [[ ! -s err ]]
    "$BIN" --target=x86_64 441.baz >out 2>err
    [[ ! -s err ]]
    local raw optimized
    raw=$(sed 's/^[[:space:]]*//' gen.s)
    optimized=$(sed 's/^[[:space:]]*//' out)
    # counts alone cannot prove the intended branches were transformed
    [[ "$raw" == *$'jmp if.10.8.code\nif.10.8.code:'* ]]
    [[ "$optimized" != *'jmp if.10.8.code'* ]]
    [[ "$optimized" == *$'je if.10.5.end\nif.10.8.code:'* ]]
    [[ "$raw" == *$'jne cmp.5.31\njmp if.5.8.code\ncmp.5.31:'* ]]
    [[ "$optimized" == *$'je if.5.8.code\ncmp.5.31:'* ]]
    [[ "$optimized" == *';   removed jumps to next code: 2'* ]]
    [[ "$optimized" == *'; inverted branches over jumps: 1'* ]]
    [[ "$raw" == *';   removed jumps to next code: 0'* ]]
    local raw_count optimized_count
    raw_count=$(grep -Ec '^[[:space:]]*j[a-z]+ ' gen.s)
    optimized_count=$(grep -Ec '^[[:space:]]*j[a-z]+ ' out)
    [[ $((raw_count - optimized_count)) -eq 3 ]]
    # identical exit status checks that branch removal preserves the result
    local temp_dir assembly
    temp_dir=$(mktemp -d /tmp/baz-jump-cli.XXXXXX)
    for assembly in gen.s out; do
        nasm -f elf64 "$assembly" -o "$temp_dir/test.o"
        ld -s -T "$SCRIPT_DIR/../../baz.ld" -o "$temp_dir/test" "$temp_dir/test.o"
        "$temp_dir/test"
    done
    rm -f "$temp_dir/test.o" "$temp_dir/test"
    rmdir "$temp_dir"
    echo "ok (jumps to next: 2, inverted: 1; jumps $raw_count -> $optimized_count; both exit 0)"
}

CLI_CHECKS_NOUB() {
    echo -n "cli --checks=noub expansion: "
    "$BIN" --checks=noub 015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,frame,alias,division,shift,overlap,stack,overflow 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=line,noub 015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,line,frame,alias,division,shift,overlap,stack,overflow 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=noub,-division,-shift,-overlap,-stack,-overflow 015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,frame,alias 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=-division,-shift,-overlap,-stack,-overflow,noub 015.baz >gen.s 2>err
    cmp -s gen.s out
    "$BIN" --checks=+division 015.baz >gen.s 2>err
    "$BIN" --checks=division 015.baz >out 2>err
    cmp -s gen.s out
    echo ok
}

CLI_CHECKS_LIST() {
    echo -n "cli --checks list: "
    "$BIN" 015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 015.baz >out 2>err
    if cmp -s gen.s out; then
        echo "FAILED. --checks=upper,lower changed nothing"
        exit 1
    fi
    "$BIN" --checks=upper,,lower, 015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 015.baz >out 2>err
    cmp -s gen.s out
    # a later option replaces the earlier ones, empty ones included
    "$BIN" --checks=upper,lower --checks= 015.baz >gen.s 2>err
    "$BIN" 015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=upper --checks=lower 015.baz >gen.s 2>err
    "$BIN" --checks=lower 015.baz >out 2>err
    cmp -s gen.s out
    echo ok
}

CLI_FILE_ERRORS() {
    echo -n "cli unreadable source and unwritable image: "
    set +e
    "$BIN" missing-source.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "cannot open file 'missing-source.baz'" err
    set +e
    "$BIN" --target=rv32i --bin=missing-directory/gen-rv32i.bin 015.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    [[ ! -s gen.s ]]
    grep -Fq "cannot write 'missing-directory/gen-rv32i.bin'" err
    echo ok
}

CLI_REGISTER_REPORT() {
    echo -n "cli --report=registers: "
    "$BIN" 847.baz >/dev/null 2>err && exit 1
    grep -Fq "register use at the failure" err
    "$BIN" 843.baz >gen.s 2>err
    "$BIN" --report=registers 843.baz >out 2>err
    [[ ! -s err ]]
    # the report only follows the code and the usual report
    cmp -s -n "$(stat -c %s gen.s)" gen.s out
    grep -Fq "register use at the peak" out
    grep -Fq "per callee, the most registers one call holds itself" out
    grep -Fq "register use at the peak" gen.s && exit 1
    # 770.baz: the peak is in a function with a body of its own
    "$BIN" --report=registers 770.baz >out 2>err
    grep -Fq "calls of functions with a body of their own save the registers" out
    grep -Fq "the peak is in a function with a body of its own" out
    echo ok
}

CLI_SOURCE_LINE_ENDS() {
    echo -n "cli crlf and tab sources: "
    # 639.baz continues strings over line ends
    sed 's/$/\r/' 639.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err
    [[ ! -s err ]]
    # the source line of an error does not show the carriage return
    sed 's/$/\r/' 848.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err && exit 1
    grep -Fq "is not a number" err
    ! grep -q $'\r' err
    # a tab stays a tab in front of the mark
    sed 's/^    /\t/' 848.baz >gen-tab.baz
    "$BIN" gen-tab.baz >gen.s 2>err && exit 1
    grep -q $'^\t   ^$' err
    rm -f gen-crlf.baz gen-tab.baz
    echo ok
}

CLI_UNINSTANTIATED_GENERICS() {
    echo -n "cli uninstantiated generics: "
    # 858.baz: two generic functions without an instance
    "$BIN" 858.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "uninstantiated generics:" gen.s
    grep -Eq '^; +no_parens$' gen.s
    grep -Eq '^; +untyped$' gen.s
    grep -Eq '^; +missing_comma$' gen.s
    echo ok
}

CLI_ADDRESS_RANGE() {
    echo -n "cli rv32i address range: "
    local target
    for target in rv32i rv32i-qemu rv32i-fpga; do
        printf 'dat big = i8[3000000000]{}\nfunc main(){\n    exit(0)\n}\n' >gen-range.baz
        set +e
        "$BIN" --target=$target --bin=gen-rv32i.bin gen-range.baz >gen.s 2>err
        local exit_code=$?
        set -e
        # fpga rejects it for its device memory, the others accept it
        if [[ $target == rv32i-fpga ]]; then
            [[ $exit_code -eq 1 ]]
            grep -Fq "of device memory" err
        else
            [[ $exit_code -eq 0 ]]
        fi
        printf 'dat big = i8[4294967000]{}\nfunc main(){\n    exit(0)\n}\n' >gen-range.baz
        rm -f gen-rv32i.bin
        set +e
        "$BIN" --target=$target --bin=gen-rv32i.bin gen-range.baz >gen.s 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        [[ ! -s gen.s && ! -e gen-rv32i.bin ]]
        grep -Fq "exceeds the RV32I address range" err
    done
    rm -f gen-range.baz gen-rv32i.bin
    echo ok
}

CLI_FPGA_MEMORY() {
    echo -n "cli rv32i-fpga memory size: "
    local memory_size=$((0x800000)) stack_size=$((0x10000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 430.baz >gen.s 2>err
    [[ ! -s err ]]
    # the variables follow the image at the next 16 byte boundary
    local image_size vars_fit
    image_size=$(stat -c %s gen-rv32i.bin)
    vars_fit=$((memory_size - stack_size - (image_size + 15) / 16 * 16))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$vars_fit 430.baz >gen.s 2>err
    [[ ! -s err ]]
    rm -f gen-rv32i.bin
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$((vars_fit + 16)) 430.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    [[ ! -s gen.s && ! -e gen-rv32i.bin ]]
    grep -Fq "exceeds the $memory_size B of device memory" err
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=$((memory_size + 16)) 430.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "the stack $((memory_size + 16)) B" err
    # a smaller memory moves the end of the stack and the limit of the vars
    local small_size=$((0x40000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size 430.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "load stack pointer to 0x4:0000" gen.s
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size --vars=$((small_size)) 430.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "exceeds the $small_size B of device memory" err
    local bad
    for bad in 0 100 0x100000000 0xfffff800 abc; do
        set +e
        "$BIN" --target=rv32i-fpga --memory=$bad 430.baz >gen.s 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        [[ ! -s gen.s ]]
    done
    echo "ok (image $image_size B, vars up to $vars_fit B)"
}

CLI_NOINLINE_REPORT() {
    echo -n "cli noinline report: "
    local target
    for target in x86_64 rv32i; do
        # 768.baz: 'sum' has an instance per array length, 'length_of' a body
        # for each of its two calls
        "$BIN" --target=$target 768.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Eq '^[;#] +sum: 2 bodies, 3 calls, [0-9]+ instructions$' gen.s
        grep -Eq '^[;#] +length_of: 2 bodies, 2 calls, [0-9]+ instructions, no reuse$' gen.s
        [[ $(grep -Ec '^[[:space:]]*call func\.sum\.' gen.s) -eq 3 ]]
        # the report comes before the optimization counts
        [[ $(grep -n 'noinline functions:' gen.s | cut -d: -f1) -lt $(grep -n 'removed jumps to next code' gen.s | cut -d: -f1) ]]
    done
    # 770.baz: one call of 'box.total'
    "$BIN" 770.baz >gen.s 2>err
    grep -Eq '^; +box.total: 1 body, 1 call, [0-9]+ instructions, no reuse$' gen.s
    echo ok
}

CLI_QEMU_STACK() {
    echo -n "cli rv32i-qemu stack size: "
    set +e
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x100000000 430.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "stack size exceeds RV32I address range" err
    echo ok
}

CLI -h 0
CLI --vars=65536 0 --help
CLI --vars=0x10000 0 --help
CLI --vars= 1 --help
CLI --vars=0 1 --help
CLI --vars=-16 1 --help
CLI --vars=17 1 --help
CLI --vars=16junk 1 --help
CLI --vars=18446744073709551616 1 --help
# more than the target addresses
CLI --vars=0x8000000000000000 1 430.baz
CLI --vars=0x100000010 1 430.baz --target=rv32i
CLI --vars=0x800000000000 0 430.baz
CLI --stack=65536 0 --help
CLI --stack=0x10000 0 --help
CLI --stack= 1 --help
CLI --stack=0 1 --help
CLI --stack=17 1 --help
CLI --stack=16junk 1 --help
CLI --no-reproduce 1 --help
CLI --checks=frame 0 015.baz
CLI --checks=alias 0 015.baz
CLI --checks=upper,lower,line,frame,alias 0 015.baz
CLI --checks=noub 0 015.baz
CLI --checks=-alias 0 948.baz
CLI --checks=upper,-alias 0 948.baz
CLI --checks=-alias,noub 0 948.baz
CLI --checks=+division 0 015.baz
CLI --checks=noub,-division 0 015.baz
CLI --checks=-noub 1 015.baz
CLI --checks=+ 1 015.baz
CLI --checks=upper 1 948.baz
CLI --checks=noub 1 948.baz
CLI --checks=unknown 1 --help
CLI --target=x86_64 0 --help
CLI --target=rv32i 0 --help
CLI --target=rv32i-qemu 0 --help
CLI --target= 1 --help
CLI --target=unknown 1 --help
CLI --bin= 1 --help
CLI --bin=gen-rv32i.bin 0 --help
CLI --report=registers 0 015.baz
CLI --report=unknown 1 --help
CLI --report=registers,unknown 1 --help
CLI --report= 1 --help
CLI --report=registers, 1 --help
CLI --report=registers,registers 0 015.baz
CLI -x 1 --help
CLI 015.baz 1 430.baz
CLI 015.baz 0 --nopt
CLI_TARGETS
CLI_BINARY_NAME
CLI_REPRODUCE_SOURCE
CLI_JUMP_OPTIMIZATIONS
CLI_CHECKS_NOUB
CLI_CHECKS_LIST
CLI_FILE_ERRORS
CLI_FPGA_MEMORY
CLI_QEMU_STACK
CLI_NOINLINE_REPORT
CLI_REGISTER_REPORT
CLI_ADDRESS_RANGE
CLI_SOURCE_LINE_ENDS
CLI_UNINSTANTIATED_GENERICS

rm -f gen.s diff.baz out err gen-rv32i.bin
