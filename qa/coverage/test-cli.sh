#!/bin/bash
# checks the command line of the compiler: option values that are accepted or
# rejected (the exit code is the expectation), the target selection, the
# binary name, and the '--nopt', '--reproduce-source' and '--versions' behavior
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
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-default.profraw" "$BIN" 0015.baz >gen.s 2>err
    [[ ! -e diff.baz ]]
    cp 0001.baz diff.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-default-nopt.profraw" "$BIN" 0015.baz --nopt >out 2>err
    cmp -s diff.baz 0001.baz
    # 'diff.baz' is written only when the reproduced source differs
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-reproduce.profraw" "$BIN" 0015.baz --reproduce-source >out 2>err
    cmp -s diff.baz 0001.baz
    cmp -s gen.s out
    rm -f diff.baz
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-reproduce-nopt.profraw" "$BIN" --reproduce-source 0015.baz --nopt >out 2>err
    [[ ! -e diff.baz ]]
    # a source named 'diff.baz' is not overwritten
    cp 0015.baz diff.baz
    "$BIN" --reproduce-source diff.baz >out 2>err
    [[ ! -s err ]]
    cmp -s diff.baz 0015.baz
    rm -f diff.baz
    echo ok
}

CLI_TARGETS() {
    echo -n "cli target selection: "
    "$BIN" 0015.baz >gen.s 2>err
    "$BIN" --target=x86_64 0015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --nopt 0015.baz >gen.s 2>err
    "$BIN" --target=x86_64 --nopt 0015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --target=rv32i 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fxq ".option norvc" gen.s
    grep -Eq '^[[:space:]]*ecall$' gen.s
    "$BIN" --target=rv32i --nopt 0426.baz >out 2>err
    [[ ! -s err ]]
    [[ $(wc -l <gen.s) -le $(wc -l <out) ]]
    # without an operating system the uart routines replace system calls
    "$BIN" --target=rv32i-qemu --stack=4096 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -Ec '^[[:space:]]*ecall$' gen.s) -eq 0 ]]
    grep -Eq '^[[:space:]]*li t0, 4096$' gen.s
    rm -f 0426-rv32i.bin 0426-rv32i-qemu.bin
    echo ok
}

CLI_BINARY_NAME() {
    echo -n "cli binary name: "
    rm -f 0426-rv32i-qemu.bin gen-rv32i.bin
    # the default is the source without extension followed by the target
    "$BIN" --target=rv32i-qemu 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ -s 0426-rv32i-qemu.bin ]]
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin 0426.baz >out 2>err
    [[ ! -s err ]]
    cmp -s 0426-rv32i-qemu.bin gen-rv32i.bin
    cmp -s gen.s out
    rm -f 0426-rv32i-qemu.bin gen-rv32i.bin
    # x86_64 writes no binary
    "$BIN" --bin=gen-rv32i.bin 0426.baz >gen.s 2>err
    [[ ! -e gen-rv32i.bin ]]
    # the image does not replace the source, also named another way
    cp 0426.baz gen-source.baz
    set +e
    "$BIN" --target=rv32i --bin=./gen-source.baz gen-source.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "invalid --bin: './gen-source.baz' is a source file" err
    cmp -s 0426.baz gen-source.baz
    rm -f gen-source.baz
    # nor an included file
    printf 'func three() res {\n    res = 3\n}\n' >gen-include.baz
    printf 'include "gen-include.baz"\n\nfunc main() {\n    exit(three())\n}\n' >gen-main.baz
    set +e
    "$BIN" --target=rv32i --bin=gen-include.baz gen-main.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "invalid --bin: 'gen-include.baz' is a source file" err
    grep -Fq "res = 3" gen-include.baz
    rm -f gen-include.baz gen-main.baz
    # x86_64 writes no image, so a source named as one is left untouched
    cp 0426.baz gen-source.baz
    "$BIN" --target=x86_64 --bin=gen-source.baz gen-source.baz >gen.s 2>err
    [[ ! -s err ]]
    cmp -s 0426.baz gen-source.baz
    rm -f gen-source.baz
    echo ok
}

CLI_JUMP_OPTIMIZATIONS() {
    echo -n "cli x86 jump optimizations: "
    "$BIN" --target=x86_64 --nopt 0437.baz >gen.s 2>err
    [[ ! -s err ]]
    "$BIN" --target=x86_64 0437.baz >out 2>err
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
    "$BIN" --checks=noub 0015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,frame,alias,division,shift,overlap,stack,overflow 0015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=line,noub 0015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,line,frame,alias,division,shift,overlap,stack,overflow 0015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=noub,-division,-shift,-overlap,-stack,-overflow 0015.baz >gen.s 2>err
    "$BIN" --checks=upper,lower,frame,alias 0015.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=-division,-shift,-overlap,-stack,-overflow,noub 0015.baz >gen.s 2>err
    cmp -s gen.s out
    "$BIN" --checks=+division 0015.baz >gen.s 2>err
    "$BIN" --checks=division 0015.baz >out 2>err
    cmp -s gen.s out
    # '-noub' turns off what 'noub' turns on, also the 'alias' that is on
    "$BIN" --checks=noub,line,-noub 0015.baz >gen.s 2>err
    "$BIN" --checks=line,-alias 0015.baz >out 2>err
    cmp -s gen.s out
    echo ok
}

CLI_CHECKS_LIST() {
    echo -n "cli --checks list: "
    # 1552.baz indexes at run time, a bounds check changes the code
    "$BIN" 1552.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 1552.baz >out 2>err
    if cmp -s gen.s out; then
        echo "FAILED. --checks=upper,lower changed nothing"
        exit 1
    fi
    "$BIN" --checks=upper,,lower, 1552.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 1552.baz >out 2>err
    cmp -s gen.s out
    # a later option replaces the earlier ones, empty ones included
    "$BIN" --checks=upper,lower --checks= 1552.baz >gen.s 2>err
    "$BIN" 1552.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=upper --checks=lower 1552.baz >gen.s 2>err
    "$BIN" --checks=lower 1552.baz >out 2>err
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
    # an error without a place in the source starts with 'error: ' like an
    # option error
    grep -Fq "error: cannot open file 'missing-source.baz'" err
    set +e
    "$BIN" --target=rv32i --bin=missing-directory/gen-rv32i.bin 0015.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    [[ ! -s gen.s ]]
    grep -Fq "cannot write file 'missing-directory/gen-rv32i.bin'" err
    # a full disk is an error, for the listing and for the image
    set +e
    "$BIN" 0015.baz >/dev/full 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: cannot write the assembly to stdout" err
    set +e
    "$BIN" --target=rv32i --bin=/dev/full 0015.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "cannot write file '/dev/full'" err
    # a source is read up to its size limit
    set +e
    "$BIN" /dev/zero >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "file '/dev/zero' is larger than 268435456 B" err
    # a source from a pipe has no canonical path
    cat 0015.baz | "$BIN" /dev/stdin >gen.s 2>err
    [[ ! -s err ]]
    # nor has an included one, it is known by its own path like the main file
    printf 'include "/proc/self/fd/0"\n\nfunc main() {\n    exit(k())\n}\n' \
        >gen-pipe-include.baz
    printf 'func k() res {\n    res = 3\n}\n' |
        "$BIN" gen-pipe-include.baz >gen.s 2>err
    [[ ! -s err ]]
    rm -f gen-pipe-include.baz
    echo ok
}

CLI_REGISTER_REPORT() {
    echo -n "cli --report=registers: "
    "$BIN" 0840.baz >/dev/null 2>err && exit 1
    grep -Fq "register use at the failure" err
    "$BIN" 0836.baz >gen.s 2>err
    "$BIN" --report=registers 0836.baz >out 2>err
    [[ ! -s err ]]
    # the report only follows the code and the usual report
    cmp -s -n "$(stat -c %s gen.s)" gen.s out
    grep -Fq "register use at the peak" out
    grep -Fq "per callee, the most registers one call holds itself" out
    grep -Fq "register use at the peak" gen.s && exit 1
    # 0763.baz: the peak is in a function with a body of its own
    "$BIN" --report=registers 0763.baz >out 2>err
    grep -Fq "calls of functions with a body of their own save the registers" out
    grep -Fq "the peak is in a function with a body of its own" out
    echo ok
}

CLI_SOURCE_LINE_ENDS() {
    echo -n "cli crlf and tab sources: "
    # 0633.baz continues strings over line ends
    sed 's/$/\r/' 0633.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err
    [[ ! -s err ]]
    # the source line of an error does not show the carriage return
    sed 's/$/\r/' 0841.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err && exit 1
    grep -Fq "is not a number" err
    grep -q $'\r' err && exit 1
    # a tab stays a tab in front of the mark
    sed 's/^    /\t/' 0841.baz >gen-tab.baz
    "$BIN" gen-tab.baz >gen.s 2>err && exit 1
    grep -q $'^\t   ^$' err
    rm -f gen-crlf.baz gen-tab.baz
    echo ok
}

CLI_UNINSTANTIATED_GENERICS() {
    echo -n "cli uninstantiated generics: "
    # 0851.baz: two generic functions without an instance
    "$BIN" 0851.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "uninstantiated generics:" gen.s
    grep -Eq '^; +untyped$' gen.s
    grep -Eq '^; +unknown_result$' gen.s
    echo ok
}

CLI_INSTANCE_COMMENT() {
    echo -n "cli comment of a generic instance: "
    # the signature names the type parameter, the comment adds the instance
    printf 'type pt {\n    x,\n}\n\n' >gen-instance.baz
    printf 'func noinline pt.put<U type>(u U) res U {\n' >>gen-instance.baz
    printf '    res = u\n}\n\nfunc main() {\n' >>gen-instance.baz
    printf '    var p = pt{1}\n' >>gen-instance.baz
    printf '    exit(int(p.put(i16(5))))\n}\n' >>gen-instance.baz
    "$BIN" gen-instance.baz >gen.s 2>err
    grep -Fq "noinline pt.put(u U) res U, instance 'pt.put<i16>'" gen.s
    rm -f gen-instance.baz
    echo ok
}

CLI_COMMENT_LOCATIONS() {
    echo -n "cli comment locations: "
    # 0906.baz includes 'include/cycle.baz'
    local target
    for target in x86_64 rv32i; do
        "$BIN" --target=$target --bin=gen-rv32i.bin 0906.baz >gen.s 2>err
        # files are named relative to the directory of the main file
        grep -Fq '[0906.baz:4:5] exit(three())' gen.s
        grep -Fq '[include/cycle.baz:4:5] res = 3' gen.s
    done
    echo ok
}

CLI_CONTROL_BYTE_FILE_NAME() {
    echo -n "cli control byte in a file name: "
    local name=$'gen\nname.baz'
    # a message escapes the line end of the name
    cp 1535.baz "$name"
    if "$BIN" "$name" >gen.s 2>err; then
        exit 1
    fi
    grep -Fq 'gen\x0aname.baz:4:15:' err
    # the comments and the panic message of the listing stay on their lines
    printf 'func main() {\n    var a = i8[2]\n    var i = 3\n    a[i] = 1\n}\n' >"$name"
    "$BIN" --checks=upper,line "$name" >gen.s 2>err
    nasm -w+error -f elf64 gen.s -o gen.o
    "$BIN" --target=rv32i --checks=upper,line "$name" >gen.s 2>err
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen.o
    # the locations of the register report name the file, also escaped
    printf 'func g(x i64) res i64 {\n    res = x * 2 + 1\n}\n' >gen-lib.baz
    printf 'include "gen-lib.baz"\n\nfunc main() {\n    var a = 1\n' >"$name"
    printf '    exit(g(a) + g(a + 1) * (a + 2))\n}\n' >>"$name"
    "$BIN" --report=registers "$name" >gen.s 2>err
    grep -Fq 'gen\x0aname.baz:' gen.s
    grep -q '^; name.baz' gen.s && exit 1
    rm -f "$name" gen-lib.baz gen.o
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
        # the bare-metal targets reject it for their device memory, linux
        # accepts it
        if [[ $target != rv32i ]]; then
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
        # data of the whole range, its address from the variables base is
        # past the range and wraps
        printf 'dat d = i8[0x100000000]\nfunc main() {\n    d[1] = 1\n}\n' \
            >gen-range.baz
        set +e
        "$BIN" --target=$target --bin=gen-rv32i.bin gen-range.baz >gen.s 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        grep -Fq "exceeds the RV32I address range" err
    done
    rm -f gen-range.baz gen-rv32i.bin
    echo ok
}

CLI_FPGA_MEMORY() {
    echo -n "cli rv32i-fpga memory size: "
    local memory_size=$((0x800000)) stack_size=$((0x10000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    # the variables follow the image at the next 16 byte boundary
    local image_size vars_fit
    image_size=$(stat -c %s gen-rv32i.bin)
    vars_fit=$((memory_size - stack_size - (image_size + 15) / 16 * 16))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$vars_fit 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    rm -f gen-rv32i.bin
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$((vars_fit + 16)) 0426.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    [[ ! -s gen.s && ! -e gen-rv32i.bin ]]
    grep -Fq "exceeds the $memory_size B of device memory" err
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=$((memory_size + 16)) 0426.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: $((memory_size + 16)) B exceeds the $memory_size B of device memory" err
    grep -Fq "use --help for usage information" err
    # a stack larger than the memory is rejected before a stack check of a
    # 'noinline' call is generated with it
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --checks=stack --stack=$((memory_size + 16)) 0992.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: $((memory_size + 16)) B exceeds the $memory_size B of device memory" err
    # the options are checked before the source, so an error in the source
    # does not hide a stack below what 'main' may use
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=16 0218.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: 16 B is smaller than the 48 B" err
    grep -Fq "cannot resolve" err && exit 1
    # a size beyond the range of a number is out of range, not a non-number
    set +e
    "$BIN" --target=rv32i-fpga --stack=99999999999999999999999 0426.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: '99999999999999999999999' is out of range" err
    # a smaller memory moves the end of the stack and the limit of the vars
    local small_size=$((0x40000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "load stack pointer to 0x4:0000" gen.s
    set +e
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size --vars=$((small_size)) 0426.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "exceeds the $small_size B of device memory" err
    local bad
    for bad in 0 100 0x100000000 0xfffff800 abc; do
        set +e
        "$BIN" --target=rv32i-fpga --memory=$bad 0426.baz >gen.s 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        [[ ! -s gen.s ]]
    done
    # the target rejects a memory that reaches its devices
    set +e
    "$BIN" --target=rv32i-fpga --memory=0x100000000 0426.baz >gen.s 2>err
    set -e
    grep -Fq "error: invalid --memory: 4294967296 B exceeds the 4294963200 B below the device addresses" err
    # the largest memory below the devices is accepted
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=0xfffff000 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    echo "ok (image $image_size B, vars up to $vars_fit B)"
}

CLI_NOINLINE_REPORT() {
    echo -n "cli noinline report: "
    local target
    for target in x86_64 rv32i; do
        # 0761.baz: 'sum' has an instance per array length, 'length_of' a body
        # for each of its two calls
        "$BIN" --target=$target --bin=gen-rv32i.bin 0761.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Eq '^[;#] +sum: 2 bodies, 3 calls, [0-9]+ instructions$' gen.s
        grep -Eq '^[;#] +length_of: 2 bodies, 2 calls, [0-9]+ instructions, no reuse$' gen.s
        [[ $(grep -Ec '^[[:space:]]*call func\.sum\.' gen.s) -eq 3 ]]
        # the report comes before the optimization counts
        [[ $(grep -n 'noinline functions:' gen.s | cut -d: -f1) -lt $(grep -n 'removed jumps to next code' gen.s | cut -d: -f1) ]]
    done
    # 0763.baz: one call of 'box.total'
    "$BIN" 0763.baz >gen.s 2>err
    grep -Eq '^; +box.total: 1 body, 1 call, [0-9]+ instructions, no reuse$' gen.s
    rm -f gen-rv32i.bin
    echo ok
}

CLI_STACK_HANDLER() {
    echo -n "cli --checks=stack handler: "
    # an operating system stops a stack overflow, no handler is written
    local target
    for target in x86_64 rv32i; do
        "$BIN" --target=$target --bin=gen-rv32i.bin --checks=stack 0992.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Fq "stack overflow handler" gen.s && exit 1
    done
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --checks=stack 0992.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "stack overflow handler (--checks=stack)" gen.s
    rm -f gen-rv32i.bin
    echo ok
}

CLI_VARS_RANGE() {
    echo -n "cli --vars past the target addresses: "
    set +e
    "$BIN" --vars=0x100000010 --target=rv32i 0426.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "invalid --vars: 4294967312 B exceeds the 4294967296 B that the target addresses" err
    grep -Fq "use --help for usage information" err
    echo ok
}

CLI_QEMU_STACK() {
    echo -n "cli rv32i-qemu stack size: "
    set +e
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x100000010 0426.baz >gen.s 2>err
    local exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: 4294967312 B exceeds the 134217728 B of device memory" err
    grep -Fq "use --help for usage information" err
    # the address space, one more than the largest 32-bit immediate
    set +e
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x100000000 0426.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "error: invalid --stack: 4294967296 B exceeds the 134217728 B of device memory" err
    grep -Fq "use --help for usage information" err
    # the 'virt' machine has 128 MiB
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x7f00000 0426.baz >gen.s 2>err
    [[ ! -s err ]]
    set +e
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x8000000 0426.baz >gen.s 2>err
    exit_code=$?
    set -e
    [[ $exit_code -eq 1 ]]
    grep -Fq "the stack 134217728 B, which exceeds the 134217728 B of device memory" err
    rm -f gen-rv32i.bin
    echo ok
}

CLI_SMALL_STACK() {
    echo -n "cli bare-metal stack below what 'main' may use: "
    # 'main' has no stack check, a helper call and a failure use 48 B
    local target exit_code
    for target in rv32i-qemu rv32i-fpga; do
        set +e
        "$BIN" --target=$target --bin=gen-rv32i.bin --stack=32 0426.baz >gen.s 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        grep -Fq "error: invalid --stack: 32 B is smaller than the 48 B that 'main' may use without a stack check" err
        grep -Fq "use --help for usage information" err
        "$BIN" --target=$target --bin=gen-rv32i.bin --stack=48 0426.baz >gen.s 2>err
        [[ ! -s err ]]
    done
    rm -f gen-rv32i.bin
    echo ok
}

# the footer counts the scratch registers of the kept version: 1224.baz keeps
# the version that holds fewer by default and the other one with 'last'
CLI_VERSIONS() {
    echo -n "cli --versions: "
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 1224.baz >gen.s 2>err
    grep -Fxq "# max scratch registers in use: 15" gen.s
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --versions=last 1224.baz >gen.s 2>err
    grep -Fxq "# max scratch registers in use: 29" gen.s
    echo ok
}

CLI_PADDING_ZERO() {
    echo -n "cli padding in array elements zeroed as one instance: "
    local target
    for target in x86_64 rv32i-fpga; do
        # 1281.baz: 4000 elements with 3 B of padding each, one zero of the
        # instance instead of a store per gap
        "$BIN" --target=$target --bin=gen-rv32i.bin 1281.baz >gen.s 2>err
        [[ ! -s err ]]
        [[ $(grep -c "zero instance for its padding: 32000 B" gen.s) -eq 1 ]]
        [[ $(grep -c "zero padding" gen.s) -eq 0 ]]
        [[ $(wc -l <gen.s) -lt 1000 ]]
    done
    echo ok
}

CLI_PADDING_ZERO_BY_TARGET() {
    echo -n "cli padding of six gaps zeroed as the target emits it smaller: "
    # 1287.baz: x86_64 zeroes the 48 B instance, rv32i each of its 6 gaps
    "$BIN" --target=x86_64 1287.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c "zero instance for its padding: 48 B" gen.s) -eq 1 ]]
    [[ $(grep -c "zero padding" gen.s) -eq 0 ]]
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 1287.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c "zero instance for its padding" gen.s) -eq 0 ]]
    [[ $(grep -c "zero padding: 3 B" gen.s) -eq 6 ]]
    echo ok
}

CLI_OPTION_MESSAGES() {
    echo -n "cli option errors name their option: "
    local arg exit_code
    for arg in --vars=0 --stack=x --memory=100 --target=x --checks=x --report=x --versions=x; do
        set +e
        "$BIN" "$arg" 0015.baz >/dev/null 2>err
        exit_code=$?
        set -e
        [[ $exit_code -eq 1 ]]
        grep -Fq "error: invalid ${arg%%=*}: " err
    done
    # each names what it lists
    "$BIN" --checks=x 0015.baz >/dev/null 2>err || true
    grep -Fq "supported checks are: " err
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
CLI --vars=0x8000000000000000 1 0426.baz
CLI --vars=0x800000000000 0 0426.baz
CLI --stack=65536 0 --help
CLI --stack=0x10000 0 --help
CLI --stack= 1 --help
CLI --stack=0 1 --help
CLI --stack=17 1 --help
CLI --stack=16junk 1 --help
CLI --no-reproduce 1 --help
CLI --checks=frame 0 0015.baz
CLI --checks=alias 0 0015.baz
CLI --checks=upper,lower,line,frame,alias 0 0015.baz
CLI --checks=noub 0 0015.baz
CLI --checks=-alias 0 0941.baz
CLI --checks=upper,-alias 0 0941.baz
CLI --checks=-alias,noub 0 0941.baz
CLI --checks=+division 0 0015.baz
CLI --checks=noub,-division 0 0015.baz
CLI --checks=-noub 0 0015.baz
CLI --checks=+ 1 0015.baz
CLI --checks=upper 1 0941.baz
CLI --checks=noub 1 0941.baz
CLI --checks=unknown 1 --help
CLI --target=x86_64 0 --help
CLI --target=rv32i 0 --help
CLI --target=rv32i-qemu 0 --help
CLI --target= 1 --help
CLI --target=unknown 1 --help
CLI --bin= 1 --help
CLI --bin=gen-rv32i.bin 0 --help
CLI --sdcard= 1 --help
CLI --sdcard=other.img 0 --help
CLI --report=registers 0 0015.baz
CLI --report=unknown 1 --help
CLI --report=registers,unknown 1 --help
CLI --report= 0 0015.baz
CLI --report=registers, 0 0015.baz
CLI --report=registers,registers 0 0015.baz
CLI --versions=smaller 0 0015.baz
CLI --versions=first 0 0015.baz
CLI --versions=last 0 0015.baz
CLI --versions= 1 --help
CLI --versions=unknown 1 --help
CLI -x 1 --help
CLI 0015.baz 1 0426.baz
CLI 0015.baz 0 --nopt
CLI_TARGETS
CLI_BINARY_NAME
CLI_REPRODUCE_SOURCE
CLI_JUMP_OPTIMIZATIONS
CLI_CHECKS_NOUB
CLI_CHECKS_LIST
CLI_FILE_ERRORS
CLI_FPGA_MEMORY
CLI_QEMU_STACK
CLI_SMALL_STACK
CLI_OPTION_MESSAGES
CLI_VARS_RANGE
CLI_STACK_HANDLER
CLI_NOINLINE_REPORT
CLI_REGISTER_REPORT
CLI_ADDRESS_RANGE
CLI_SOURCE_LINE_ENDS
CLI_UNINSTANTIATED_GENERICS
CLI_COMMENT_LOCATIONS
CLI_INSTANCE_COMMENT
CLI_CONTROL_BYTE_FILE_NAME
CLI_VERSIONS
CLI_PADDING_ZERO
CLI_PADDING_ZERO_BY_TARGET

rm -f gen.s diff.baz out err gen-rv32i.bin
