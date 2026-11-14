#!/bin/bash
# checks the command line of the compiler: option values that are accepted or
# rejected (the exit code is the expectation), the target selection, the
# binary name, and the '--nopt', '--reproduce-source' and '--versions' behavior;
# also what a case of 'cases.sh' cannot express, e.g. a comment, a report line
# or an instruction that a listing must have, and programs that are compiled
# but not run; needs 'nasm', 'ld' and 'llvm-mc'
#
# usage: test-cli.sh
set -eE
# names the failed command; those under 'set +e' fail on purpose
trap '[[ $- != *e* ]] || echo "FAILED. line $LINENO: $BASH_COMMAND" >&2' ERR

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/tests"

BIN="$SCRIPT_DIR/../../baz"
export LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-%p.profraw"

# the programs that a case links and runs, also removed when a case fails
WORK=$(mktemp -d)
trap 'rm -rf -- "$WORK"' EXIT

# runs the compiler for target '$1' with the other arguments, an rv32i target
# writes its image to 'gen-rv32i.bin', x86_64 takes no '--bin'
baz_at() {
    local target=$1
    shift
    if [[ $target == x86_64 ]]; then
        "$BIN" --target="$target" "$@"
    else
        "$BIN" --target="$target" --bin=gen-rv32i.bin "$@"
    fi
}

# runs a command that must exit with 1, e.g. a rejected compile
expect_exit_1() {
    local exit_code=0
    "$@" || exit_code=$?
    # note: a failure returned names the line of the caller, not this one
    [[ $exit_code -eq 1 ]] || return 1
}

# fails the case where a command must not succeed, naming the line of the
# caller as the trap does, e.g. 'grep -Fq text out && fail'
fail() {
    echo "FAILED. line ${BASH_LINENO[0]}" >&2
    exit 1
}

CLI() {
    echo -n "cli $1: "
    set +e
    # note: '%p' since cases share their first argument, e.g. '--checks=noub'
    LLVM_PROFILE_FILE="$SCRIPT_DIR/cli-$1-%p.profraw" "$BIN" "$1" "${@:3}" >/dev/null 2>err
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
    # a matching reproduction writes no 'diff.baz', so a source of that name
    # stays
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
    "$BIN" --target=rv32i 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fxq ".option norvc" gen.s
    grep -Eq '^[[:space:]]*ecall$' gen.s
    "$BIN" --target=rv32i --nopt 0418.baz >out 2>err
    [[ ! -s err ]]
    [[ $(wc -l <gen.s) -le $(wc -l <out) ]]
    # without an operating system the uart routines replace system calls
    "$BIN" --target=rv32i-qemu --stack=4096 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -Ec '^[[:space:]]*ecall$' gen.s) -eq 0 ]]
    grep -Fxq '# load stack pointer to 0x87e0:0000' gen.s
    rm -f 0418-rv32i.bin 0418-rv32i-qemu.bin
    echo ok
}

CLI_BINARY_NAME() {
    echo -n "cli binary name: "
    rm -f 0418-rv32i-qemu.bin gen-rv32i.bin
    # the default is the source without extension followed by the target
    "$BIN" --target=rv32i-qemu 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ -s 0418-rv32i-qemu.bin ]]
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin 0418.baz >out 2>err
    [[ ! -s err ]]
    cmp -s 0418-rv32i-qemu.bin gen-rv32i.bin
    cmp -s gen.s out
    rm -f 0418-rv32i-qemu.bin gen-rv32i.bin
    # x86_64 writes no binary, so '--bin' is rejected like the other options
    # a target does not read
    expect_exit_1 "$BIN" --bin=gen-rv32i.bin 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --bin: target 'x86_64' does not read it, the targets that do are: rv32i, rv32i-qemu, rv32i-fpga" err
    [[ ! -e gen-rv32i.bin ]]
    # the image does not replace the source, also named another way
    cp 0418.baz gen-source.baz
    expect_exit_1 "$BIN" --target=rv32i --bin=./gen-source.baz gen-source.baz >gen.s 2>err
    grep -Fq "invalid --bin: './gen-source.baz' is a source file" err
    cmp -s 0418.baz gen-source.baz
    rm -f gen-source.baz
    # nor an included file
    printf 'func three() res {\n    res = 3\n}\n' >gen-include.baz
    printf 'include "gen-include.baz"\n\nfunc main() {\n    exit(three())\n}\n' >gen-main.baz
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-include.baz gen-main.baz >gen.s 2>err
    grep -Fq "invalid --bin: 'gen-include.baz' is a source file" err
    grep -Fq "res = 3" gen-include.baz
    rm -f gen-include.baz gen-main.baz
    # the sd card that the program writes is no source file either
    cp 0418.baz gen-source.baz
    expect_exit_1 "$BIN" --target=x86_64 --sdcard=./gen-source.baz gen-source.baz >gen.s 2>err
    grep -Fq "invalid --sdcard: './gen-source.baz' is a source file" err
    printf 'func three() res {\n    res = 3\n}\n' >gen-include.baz
    printf 'include "gen-include.baz"\n\nfunc main() {\n    exit(three())\n}\n' >gen-main.baz
    expect_exit_1 "$BIN" --target=rv32i --sdcard=gen-include.baz gen-main.baz >gen.s 2>err
    grep -Fq "invalid --sdcard: 'gen-include.baz' is a source file" err
    rm -f gen-source.baz gen-include.baz gen-main.baz
    # nor the sd card that the program reads
    head -c 4096 /dev/zero >gen-sdcard.img
    expect_exit_1 "$BIN" --target=rv32i --bin=./gen-sdcard.img --sdcard=gen-sdcard.img 0418.baz >gen.s 2>err
    grep -Fq "invalid --bin: './gen-sdcard.img' is the sd card image" err
    [[ $(stat -c %s gen-sdcard.img) -eq 4096 ]]
    # also through a hard link, which has a path of its own
    ln -f gen-sdcard.img gen-sdcard-link.img
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-sdcard-link.img --sdcard=gen-sdcard.img 0418.baz >gen.s 2>err
    grep -Fq "invalid --bin: 'gen-sdcard-link.img' is the sd card image" err
    [[ $(stat -c %s gen-sdcard.img) -eq 4096 ]]
    rm -f gen-sdcard.img gen-sdcard-link.img
    # also when the card does not exist yet
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-sdcard.img --sdcard=./gen-sdcard.img 0418.baz >gen.s 2>err
    grep -Fq "invalid --bin: 'gen-sdcard.img' is the sd card image" err
    [[ ! -e gen-sdcard.img ]]
    echo ok
}

CLI_RELOADS() {
    echo -n "cli rv32i reload after a jump to the next line: "
    # the 'j' to the end of a folded 'if' falls through, removed by the jump
    # optimizations, so its label no longer keeps the load after the store
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --nopt 1639.baz >gen.s 2>err
    [[ ! -s err ]]
    "$BIN" --target=rv32i --bin=gen-rv32i.bin 1639.baz >out 2>err
    [[ ! -s err ]]
    grep -Fq '# removed reloads after stores: 0' gen.s
    grep -Fq '# removed reloads after stores: 1' out
    rm -f gen-rv32i.bin
    echo ok
}

CLI_RETESTS() {
    echo -n "cli x86 test after setcc to memory: "
    # the byte that 'setl' wrote is not compared with zero again, the branch
    # takes the flags of the comparison
    "$BIN" --target=x86_64 3134.baz >gen.s 2>err
    [[ ! -s err ]]
    local listing
    listing=$(sed 's/^[[:space:]]*//' gen.s)
    [[ "$listing" == *$'\nsetl byte ['* ]]
    [[ "$listing" != *'cmp byte ['* ]]
    [[ "$listing" == *$'\njge if.9.5.end\n'* ]]
    [[ "$listing" == *';    removed tests after setcc: 1'* ]]
    nasm -w+error -f elf64 gen.s -o "$WORK/test.o"
    ld -s -T "$SCRIPT_DIR/../../baz.ld" -o "$WORK/test" "$WORK/test.o"
    expect_exit_1 "$WORK/test" </dev/null
    echo ok
}

CLI_SHARED_BASE_COPY() {
    echo -n "cli rv32i copy between far variables through one base: "
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --vars=0x40000 3135.baz >gen.s 2>err
    [[ ! -s err ]]
    local listing
    listing=$(sed 's/^[[:space:]]*//' gen.s)
    # one 'lui' and 'add' for both addresses, then the copy is done
    [[ "$listing" == *$'source and destination share base t0\nlui t0, 24\nadd t0, t0, s0\nlw t1, -336(t0)\nsw t1, -332(t0)\n# [3135.baz:10:9] free scratch register t1\n# [3135.baz:10:9] free scratch register t0\n# [3135.baz:11:5] exit(b)'* ]]
    rm -f gen-rv32i.bin
    echo ok
}

# every instruction from 'main' on, the run-time routines included, is
# indented one level; labels, comments and directives are not instructions
CLI_ROUTINE_INDENT() {
    echo -n "cli rv32i routines indented like main: "
    local target
    for target in rv32i rv32i-qemu rv32i-fpga; do
        "$BIN" --target="$target" --bin=gen-rv32i.bin --checks=noub,line 3136.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -q '^\.Lbaz_multiply_checked:$' gen.s
        awk '/^main:$/ { in_main = 1; next }
            in_main && !/^ *#/ && !/^ *[A-Za-z_.$0-9]+:$/ && !/^\./ && !/^$/ && !/^    [^ ]/ { bad = 1 }
            END { exit !in_main || bad }' gen.s
    done
    rm -f gen-rv32i.bin
    echo ok
}

CLI_JUMP_OPTIMIZATIONS() {
    echo -n "cli x86 jump optimizations: "
    "$BIN" --target=x86_64 --nopt 0429.baz >gen.s 2>err
    [[ ! -s err ]]
    "$BIN" --target=x86_64 0429.baz >out 2>err
    [[ ! -s err ]]
    local raw optimized
    raw=$(sed 's/^[[:space:]]*//' gen.s)
    optimized=$(sed 's/^[[:space:]]*//' out)
    # counts alone cannot prove the intended branches were transformed
    [[ "$raw" == *$'jmp if.10.5.code\nif.10.5.code:'* ]]
    [[ "$optimized" != *'jmp if.10.5.code'* ]]
    [[ "$optimized" == *$'je if.10.5.end\nif.10.5.code:'* ]]
    [[ "$raw" == *$'jne cmp.5.31\njmp if.5.5.code\ncmp.5.31:'* ]]
    [[ "$optimized" == *$'je if.5.5.code\ncmp.5.31:'* ]]
    [[ "$optimized" == *';   removed jumps to next code: 3'* ]]
    [[ "$optimized" == *'; inverted branches over jumps: 1'* ]]
    [[ "$raw" == *';   removed jumps to next code: 0'* ]]
    local raw_count optimized_count
    raw_count=$(grep -Ec '^[[:space:]]*j[a-z]+ ' gen.s)
    optimized_count=$(grep -Ec '^[[:space:]]*j[a-z]+ ' out)
    [[ $((raw_count - optimized_count)) -eq 4 ]]
    # identical exit status checks that branch removal preserves the result
    local assembly
    for assembly in gen.s out; do
        nasm -w+error -f elf64 "$assembly" -o "$WORK/test.o"
        ld -s -T "$SCRIPT_DIR/../../baz.ld" -o "$WORK/test" "$WORK/test.o"
        "$WORK/test"
    done
    echo "ok (jumps to next: 3, inverted: 1; jumps $raw_count -> $optimized_count; both exit 0)"
}

CLI_CHECKS_NOUB() {
    echo -n "cli --checks=noub expansion: "
    # each check alone changes the code of 2145.baz on rv32i-fpga, the one
    # target with all of them, so a check missing from a list is seen
    local fpga=(--target=rv32i-fpga --bin=gen-rv32i.bin)
    "$BIN" "${fpga[@]}" 2145.baz >out 2>err
    local check
    for check in upper lower frame division shift overlap stack overflow; do
        "$BIN" "${fpga[@]}" --checks="$check" 2145.baz >gen.s 2>err
        if cmp -s gen.s out; then
            echo "FAILED. --checks=$check changed nothing"
            exit 1
        fi
    done
    "$BIN" "${fpga[@]}" --checks=noub 2145.baz >gen.s 2>err
    "$BIN" "${fpga[@]}" --checks=upper,lower,frame,division,shift,overlap,stack,overflow 2145.baz >out 2>err
    cmp -s gen.s out
    "$BIN" "${fpga[@]}" --checks=line,noub 2145.baz >gen.s 2>err
    "$BIN" "${fpga[@]}" --checks=upper,lower,line,frame,division,shift,overlap,stack,overflow 2145.baz >out 2>err
    cmp -s gen.s out
    "$BIN" "${fpga[@]}" --checks=noub,-division,-shift,-overlap,-stack,-overflow 2145.baz >gen.s 2>err
    "$BIN" "${fpga[@]}" --checks=upper,lower,frame 2145.baz >out 2>err
    cmp -s gen.s out
    "$BIN" "${fpga[@]}" --checks=-division,-shift,-overlap,-stack,-overflow,noub 2145.baz >gen.s 2>err
    cmp -s gen.s out
    "$BIN" "${fpga[@]}" --checks=+division 2145.baz >gen.s 2>err
    "$BIN" "${fpga[@]}" --checks=division 2145.baz >out 2>err
    cmp -s gen.s out
    # '-noub' turns off what 'noub' turns on
    "$BIN" "${fpga[@]}" --checks=noub,line,-noub 2145.baz >gen.s 2>err
    "$BIN" "${fpga[@]}" --checks=line 2145.baz >out 2>err
    cmp -s gen.s out
    echo ok
}

CLI_CHECKS_LIST() {
    echo -n "cli --checks list: "
    # 1534.baz indexes at run time, a bounds check changes the code
    "$BIN" 1534.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 1534.baz >out 2>err
    if cmp -s gen.s out; then
        echo "FAILED. --checks=upper,lower changed nothing"
        exit 1
    fi
    "$BIN" --checks=upper,,lower, 1534.baz >gen.s 2>err
    "$BIN" --checks=upper,lower 1534.baz >out 2>err
    cmp -s gen.s out
    # a later option replaces the earlier ones, empty ones included
    "$BIN" --checks=upper,lower --checks= 1534.baz >gen.s 2>err
    "$BIN" 1534.baz >out 2>err
    cmp -s gen.s out
    "$BIN" --checks=upper --checks=lower 1534.baz >gen.s 2>err
    "$BIN" --checks=lower 1534.baz >out 2>err
    cmp -s gen.s out
    echo ok
}

CLI_FILE_ERRORS() {
    echo -n "cli unreadable source and unwritable image: "
    expect_exit_1 "$BIN" missing-source.baz >gen.s 2>err
    # an error without a place in the source starts with 'error: ' like an
    # option error
    grep -Fq "error: cannot open file 'missing-source.baz'" err
    expect_exit_1 "$BIN" --target=rv32i --bin=missing-directory/gen-rv32i.bin 0015.baz >gen.s 2>err
    [[ ! -s gen.s ]]
    grep -Fq "cannot write file 'missing-directory/gen-rv32i.bin'" err
    # a full disk is an error, for the listing and for the image
    expect_exit_1 "$BIN" 0015.baz >/dev/full 2>err
    grep -Fq "error: cannot write the assembly to stdout" err
    # and leaves no image
    rm -f gen-full.bin
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-full.bin 0015.baz >/dev/full 2>err
    [[ ! -e gen-full.bin ]]
    # through a symbolic link the file it names is removed, the link stays
    : >gen-full.bin
    ln -sf gen-full.bin gen-link.bin
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-link.bin 0015.baz >/dev/full 2>err
    [[ -L gen-link.bin ]]
    [[ ! -e gen-full.bin ]]
    rm -f gen-link.bin
    # a pipe is not removed, nor is a device, e.g. '/dev/null'; the pipe is
    # open for reading too, so the compiler does not wait for a reader
    rm -f gen-pipe.bin
    mkfifo gen-pipe.bin
    exec 3<>gen-pipe.bin
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-pipe.bin 0015.baz >/dev/full 2>err
    exec 3<&-
    [[ -p gen-pipe.bin ]]
    rm -f gen-pipe.bin
    # an image that cannot be written whole is removed too; the file size
    # limit stands for the full disk, its signal ignored so the write fails,
    # and the message is read from a pipe, which has no limit
    rm -f gen-full.bin
    local message
    message=$(
        trap '' XFSZ
        ulimit -f 0
        "$BIN" --target=rv32i --bin=gen-full.bin 0015.baz 2>&1 >/dev/null
    ) && fail
    [[ $message == *"error: cannot write file 'gen-full.bin'"* ]]
    [[ ! -e gen-full.bin ]]
    # an image that cannot be opened is not the compiler's to remove
    : >gen-full.bin
    chmod a-w gen-full.bin
    expect_exit_1 "$BIN" --target=rv32i --bin=gen-full.bin 0015.baz >gen.s 2>err
    grep -Fq "cannot write file 'gen-full.bin'" err
    [[ -e gen-full.bin ]]
    rm -f gen-full.bin
    expect_exit_1 "$BIN" --target=rv32i --bin=/dev/full 0015.baz >gen.s 2>err
    [[ ! -s gen.s ]]
    grep -Fq "cannot write file '/dev/full'" err
    # a message that cannot be written still exits with 1, the help too
    expect_exit_1 "$BIN" --help >/dev/full 2>err
    # the version is the tag and commit of the build, written like the help
    "$BIN" --version >gen.s 2>err
    grep -Eq '^baz [^ ]+$' gen.s
    expect_exit_1 "$BIN" --version >/dev/full 2>err
    grep -Fq "cannot write the version to stdout" err
    # a help shorter than a block is still checked, e.g. a closed stdout
    expect_exit_1 "$BIN" --help 1>&- 2>err
    expect_exit_1 "$BIN" missing-source.baz >gen.s 2>/dev/full
    # a source is read up to its size limit
    expect_exit_1 "$BIN" /dev/zero >gen.s 2>err
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
    # in a working directory that was removed '../0015.baz' still opens, but
    # has no path: an error of the user, not an internal one
    mkdir gen-removed
    (
        cd gen-removed
        rmdir ../gen-removed
        expect_exit_1 "$BIN" ../0015.baz >../gen.s 2>../err
    )
    [[ ! -s gen.s ]]
    grep -Fxq "error: cannot resolve file '../0015.baz', the working directory is not found" err
    echo ok
}

CLI_REGISTER_REPORT() {
    echo -n "cli --report=registers: "
    "$BIN" 0829.baz >/dev/null 2>err && fail
    grep -Fq "register use at the failure" err
    "$BIN" 0825.baz >gen.s 2>err
    "$BIN" --report=registers 0825.baz >out 2>err
    [[ ! -s err ]]
    # the report only follows the code and the usual report
    cmp -s -n "$(stat -c %s gen.s)" gen.s out
    grep -Fq "register use at the peak" out
    grep -Fq "per callee, the most registers one call holds itself" out
    grep -Fq "register use at the peak" gen.s && fail
    # 0753.baz: the peak is in a function with a body of its own
    "$BIN" --report=registers 0753.baz >out 2>err
    grep -Fq "calls of functions with a body of their own save the registers" out
    grep -Fq "the peak is in a function with a body of its own" out
    # 2800.baz: also where the peak is in a call inlined in that body
    "$BIN" --report=registers 2800.baz >out 2>err
    grep -Fq "add3 called at 9:11" out
    grep -Fq "the peak is in a function with a body of its own" out
    # 2106.baz: the peak is in a global initializer, outside every function
    "$BIN" --report=registers 2106.baz >out 2>err
    grep -Fq "register use at the peak: 2 of" out
    grep -Fq "global initializers" out
    # 0136.baz: no register is held on x86_64, the peak is in 'main'
    "$BIN" --report=registers 0136.baz >out 2>err
    grep -Fq "register use at the peak: 0 of" out
    grep -Eq '^; +0  main$' out
    grep -Fq "global initializers" out && fail
    # the total leaves out the registers the backend holds itself, 13 of the
    # 14 scratch registers on x86_64 and 29 of 30 on rv32i here
    "$BIN" --report=registers 0402.baz >out 2>err
    grep -Fq "register use at the peak: 3 of 13 registers live" out
    "$BIN" --target=rv32i --report=registers 0001.baz >out 2>err
    grep -Fq "register use at the peak: 4 of 29 registers live" out
    echo ok
}

CLI_SOURCE_LINE_ENDS() {
    echo -n "cli crlf and tab sources: "
    # 0624.baz continues strings over line ends
    sed 's/$/\r/' 0624.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err
    [[ ! -s err ]]
    # the source line of an error does not show the carriage return
    sed 's/$/\r/' 0830.baz >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err && fail
    grep -Fq "is not a number" err
    grep -q $'\r' err && fail
    # a tab stays a tab in front of the mark
    sed 's/^    /\t/' 0830.baz >gen-tab.baz
    "$BIN" gen-tab.baz >gen.s 2>err && fail
    grep -q $'^\t   ^$' err
    # a carriage return without a line feed is rejected, also in a
    # character literal, and shown escaped
    printf "func main() {\r\n    var c = '\r'\r\n    exit(c)\r\n}\r\n" \
        >gen-crlf.baz
    "$BIN" gen-crlf.baz >gen.s 2>err && fail
    grep -Fq "a carriage return is not followed by a line feed" err
    grep -Fq "var c = '\\x0d'" err
    rm -f gen-crlf.baz gen-tab.baz
    echo ok
}

# invalid UTF-8 and DEL in the error mark: a lead byte without its
# continuation, an invalid lead byte and DEL are escaped and take the four
# columns of their '\xNN'
CLI_ERROR_LINE_BYTES() {
    echo -n "cli error line with invalid UTF-8 and DEL: "
    printf 'func main() {\n    var s = "\xc3A\xc3\xc3\xf8\x7f\xc3\xa9" + y\n}\n' \
        >gen-bytes.baz
    "$BIN" gen-bytes.baz >gen.s 2>err && fail
    grep -Fq '\x7f' err
    grep -Fq '"\xc3A\xc3\xc3\xf8\x7f' err
    grep -Fxq '            ^~~~~~~~~~~~~~~~~~~~~~~~' err
    # a DEL in a name is neither a letter nor a digit
    printf 'func main() {\n    var a\x7f = 1\n}\n' >gen-bytes.baz
    "$BIN" gen-bytes.baz >gen.s 2>err && fail
    grep -Fq "'a\\x7f' is not a valid name" err
    rm -f gen-bytes.baz
    echo ok
}

CLI_UNINSTANTIATED_GENERICS() {
    echo -n "cli uninstantiated generics: "
    # 0840.baz: two generic functions and a method of a generic type without
    # an instance
    "$BIN" 0840.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "uninstantiated generics:" gen.s
    grep -Eq '^; +untyped$' gen.s
    grep -Eq '^; +unknown_result$' gen.s
    grep -Eq '^; +list.find$' gen.s
    # 2481.baz: a called method, a method without type parameters and a
    # called function are not listed, the others once in definition order
    "$BIN" 2481.baz >gen.s 2>err
    [[ ! -s err ]]
    local listed
    listed=$(sed -n '/uninstantiated generics:/,/^;$/p' gen.s | sed 's/^; *//')
    [[ "$listed" == $'uninstantiated generics:\nbox.other\nlater' ]]
    echo ok
}

CLI_TYPE_LAYOUT_COMMENT() {
    echo -n "cli layout comment of a type: "
    # 2482.baz: 'b i32[3]' at offset 4 is an array of length 3
    "$BIN" 2482.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Eq '^;\[2482.baz:4:1\] +a : +0 : +1 : +no : +$' gen.s
    grep -Eq '^;\[2482.baz:4:1\] +b : +4 : +12 : +yes : +3$' gen.s
    echo ok
}

CLI_ZERO_COMMENTS() {
    echo -n "cli comments of the zeroed parts of an instance: "
    # 2484.baz: 'rec{1, 2, 3}' has padding only, 'rec{4}' and 'rec{5}' have
    # fields that are not listed
    "$BIN" 2484.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq '[2484.baz:11:12] zero padding: 3 B' gen.s
    grep -Fq '[2484.baz:13:12] zero remaining fields: 11 B' gen.s
    grep -Fq '[2484.baz:16:13] zero remaining fields: 11 B' gen.s
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
    # 0895.baz includes 'include/cycle.baz'
    local target
    for target in x86_64 rv32i; do
        baz_at "$target" 0895.baz >gen.s 2>err
        # files are named relative to the directory of the main file
        grep -Fq '[0895.baz:4:5] exit(three())' gen.s
        grep -Fq '[include/cycle.baz:4:5] res = 3' gen.s
    done
    # a file included by its absolute path and a main file named with a
    # directory or an absolute path give the same names
    printf 'func three() res {\n    res = 3\n}\n' >gen-abs-lib.baz
    printf 'include "%s/gen-abs-lib.baz"\n\nfunc main() {\n    exit(three())\n}\n' \
        "$PWD" >gen-abs-main.baz
    local main
    for main in gen-abs-main.baz ../tests/gen-abs-main.baz \
        "$PWD/gen-abs-main.baz"; do
        "$BIN" "$main" >gen.s 2>err
        grep -Fq "[gen-abs-lib.baz:2:5] res = 3" gen.s
        grep -Fq "[gen-abs-main.baz:4:5] exit(three())" gen.s
        # the register report names the files the same way
        "$BIN" --report=registers "$main" >gen.s 2>err
        grep -Fq "rdi gen-abs-main.baz:4:5 (named)" gen.s
    done
    # the panic message names the file the same way
    printf 'func main() {\n    var a = i8[2]\n    var i = 3\n    a[i] = 1\n}\n' \
        >gen-abs-panic.baz
    "$BIN" --checks=upper,line "$PWD/gen-abs-panic.baz" >gen.s 2>err
    grep -Fq 'panic: bounds at gen-abs-panic.baz:' gen.s
    # an error in a file included by an absolute path with a '..' names the
    # file without it
    printf 'func three() res {\n    res = nope\n}\n' >gen-abs-lib.baz
    printf 'include "%s/include/../gen-abs-lib.baz"\n\nfunc main() {\n    exit(three())\n}\n' \
        "$PWD" >gen-abs-main.baz
    "$BIN" gen-abs-main.baz >gen.s 2>err && fail
    grep -Fq "$PWD/gen-abs-lib.baz:2:11:" err
    rm -f gen-abs-lib.baz gen-abs-main.baz gen-abs-panic.baz
    echo ok
}

CLI_CONTROL_BYTE_FILE_NAME() {
    echo -n "cli control byte in a file name: "
    local name=$'gen\nname.baz'
    # a message escapes the line end of the name
    cp 1517.baz "$name"
    "$BIN" "$name" >gen.s 2>err && fail
    grep -Fq 'gen\x0aname.baz:4:15:' err
    # the comments and the panic message of the listing stay on their lines
    printf 'func main() {\n    var a = i8[2]\n    var i = 3\n    a[i] = 1\n}\n' >"$name"
    "$BIN" --checks=upper,line "$name" >gen.s 2>err
    nasm -w+error -f elf64 gen.s -o gen.o
    # the panic message escapes the line end like the messages do
    ld -s -T "$SCRIPT_DIR/../../baz.ld" -o gen gen.o
    ./gen >out 2>&1 || true
    grep -Fq 'panic: bounds at gen\x0aname.baz:4' out
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --checks=upper,line "$name" >gen.s 2>err
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj gen.s -o gen.o
    # the locations of the register report name the file, also escaped
    printf 'func g(x i64) res i64 {\n    res = x * 2 + 1\n}\n' >gen-lib.baz
    printf 'include "gen-lib.baz"\n\nfunc main() {\n    var a = 1\n' >"$name"
    printf '    exit(g(a) + g(a + 1) * (a + 2))\n}\n' >>"$name"
    "$BIN" --report=registers "$name" >gen.s 2>err
    grep -Fq 'gen\x0aname.baz:' gen.s
    grep -q '^; name.baz' gen.s && fail
    rm -f "$name" gen-lib.baz gen.o gen
    echo ok
}

CLI_INCLUDE_THROUGH_LINK() {
    echo -n "cli include through a symbolic link: "
    # a '..' after a link to a directory is the parent of the link's target,
    # as the file system resolves it
    rm -rf gen-link-dir gen-link gen-e.baz
    mkdir -p gen-link-dir/sub
    printf 'func three() res {\n    res = 3\n}\n' >gen-link-dir/gen-lib.baz
    printf 'include "../gen-lib.baz"\n\nfunc main() {\n    exit(three())\n}\n' \
        >gen-link-dir/sub/gen-main.baz
    ln -s gen-link-dir/sub gen-link
    "$BIN" gen-link/gen-main.baz >gen.s 2>err
    grep -Fq '[../gen-lib.baz:2:5] res = 3' gen.s
    # the listing and the panic name the file opened, not the one that the
    # path names without the link
    printf 'func three() res {\n    res = 4\n}\n' >gen-lib.baz
    printf 'func three() res {\n    var a = i8[2]\n    var i = 3\n    a[i] = 1\n    res = 3\n}\n' \
        >gen-link-dir/gen-lib.baz
    printf 'include "gen-link/../gen-lib.baz"\n\nfunc main() {\n    exit(three())\n}\n' \
        >gen-main.baz
    "$BIN" --checks=upper,line gen-main.baz >gen.s 2>err
    grep -Fq '[gen-link-dir/gen-lib.baz:5:5] res = 3' gen.s
    grep -Fq 'panic: bounds at gen-link-dir/gen-lib.baz:' gen.s
    # a chain of includes across directories keeps its paths short: the
    # error names 'gen-link-dir/gen-c.baz', not 'gen-link-dir/sub/../gen-c.baz'
    printf 'include "sub/gen-b.baz"\n' >gen-link-dir/gen-a.baz
    printf 'include "../gen-c.baz"\n' >gen-link-dir/sub/gen-b.baz
    printf 'func f() res {\n    res = nope\n}\n' >gen-link-dir/gen-c.baz
    printf 'include "gen-link-dir/gen-a.baz"\n\nfunc main() {\n    exit(f())\n}\n' \
        >gen-main.baz
    "$BIN" gen-main.baz >gen.s 2>err && fail
    grep -Fq 'gen-link-dir/gen-c.baz:2:11:' err
    # also after a link: the error names 'gen-link-dir/sub/gen-d.baz', not
    # 'gen-link/../sub/gen-d.baz'
    printf 'include "sub/gen-d.baz"\n' >gen-link-dir/gen-c.baz
    printf 'func f() res {\n    res = nope\n}\n' >gen-link-dir/sub/gen-d.baz
    printf 'include "gen-link/gen-b.baz"\n\nfunc main() {\n    exit(f())\n}\n' \
        >gen-main.baz
    "$BIN" gen-main.baz >gen.s 2>err && fail
    grep -Fq 'gen-link-dir/sub/gen-d.baz:2:11:' err
    # the includes of a file reached by a '..' after a link are relative to
    # the directory it was opened from, also where a file of that name next
    # to the link is the same file: 'gen-link/../gen-e.baz' is
    # 'gen-link-dir/gen-e.baz', its 'gen-lib.baz' 'gen-link-dir/gen-lib.baz'
    printf 'include "gen-lib.baz"\n' >gen-link-dir/gen-e.baz
    printf 'include "gen-link/../gen-e.baz"\n\nfunc main() {\n    exit(three())\n}\n' \
        >gen-main.baz
    ln gen-link-dir/gen-e.baz gen-e.baz
    "$BIN" gen-main.baz >gen.s 2>err
    grep -Fq '[gen-link-dir/gen-lib.baz:5:5] res = 3' gen.s
    rm gen-e.baz
    ln -s gen-link-dir/gen-e.baz gen-e.baz
    "$BIN" gen-main.baz >gen.s 2>err
    grep -Fq '[gen-link-dir/gen-lib.baz:5:5] res = 3' gen.s
    rm gen-e.baz
    # a link to its own directory does not grow the paths of a chain past
    # the limit of links in one path
    ln -s . gen-link-dir/self
    local i
    for i in {1..41}; do
        printf 'include "self/gen-s%d.baz"\n' $((i + 1)) >gen-link-dir/gen-s$i.baz
    done
    # note: 41 links, Linux follows at most 40 in one path
    printf 'func f() res {\n    res = 3\n}\n' >gen-link-dir/gen-s42.baz
    printf 'include "gen-link-dir/gen-s1.baz"\n\nfunc main() {\n    exit(f())\n}\n' \
        >gen-main.baz
    "$BIN" gen-main.baz >gen.s 2>err
    # a path through a file names no file, as the file system says
    printf 'include "gen-lib.baz/../gen-lib.baz"\n' >gen-main.baz
    "$BIN" gen-main.baz >gen.s 2>err && fail
    grep -Fq "cannot open file 'gen-lib.baz/../gen-lib.baz'" err
    # the file system resolves the link also where a directory above the
    # working directory cannot be searched
    mkdir -p gen-perm/dir/sub/x
    ln -s sub/x gen-perm/dir/link
    printf 'include "link/../lib.baz"\ninclude "lib.baz"\n\nfunc main() {\n    exit(three() + four())\n}\n' \
        >gen-perm/dir/main.baz
    printf 'func four() res {\n    res = 4\n}\n' >gen-perm/dir/lib.baz
    printf 'func three() res {\n    res = 3\n}\n' >gen-perm/dir/sub/lib.baz
    set +e
    (cd gen-perm/dir && chmod a-x .. && "$BIN" main.baz >gen.s 2>err)
    exit_code=$?
    set -e
    chmod a+x gen-perm
    [[ $exit_code -eq 0 ]]
    # the listing names the file opened, not 'lib.baz' as a lexical '..' does
    grep -Fq '[link/../lib.baz:2:5] res = 3' gen-perm/dir/gen.s
    rm -rf gen-perm
    rm -rf gen-link-dir gen-link gen-e.baz gen-lib.baz gen-main.baz
    echo ok
}

CLI_ADDRESS_RANGE() {
    echo -n "cli rv32i address range: "
    local target
    for target in rv32i rv32i-qemu rv32i-fpga; do
        printf 'dat big = i8[3000000000]\nfunc main(){\n    exit(0)\n}\n' >gen-range.baz
        set +e
        baz_at "$target" gen-range.baz >gen.s 2>err
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
        # the dats and the default 64 KiB of variables fit, the code does not
        printf 'dat big = i8[0xfffefff0]\nfunc main(){\n    exit(0)\n}\n' >gen-range.baz
        rm -f gen-rv32i.bin
        expect_exit_1 baz_at "$target" gen-range.baz >gen.s 2>err
        [[ ! -s gen.s && ! -e gen-rv32i.bin ]]
        grep -Fq "code, data and variables use" err
        # data of the whole range leaves no room, it is rejected where it is
        # declared, before its address from the variables base would wrap
        printf 'dat d = i8[0x100000000]\nfunc main() {\n    d[1] = 1\n}\n' \
            >gen-range.baz
        expect_exit_1 baz_at "$target" gen-range.baz >gen.s 2>err
        grep -Fq "data and variables use 4294967296 B, which reaches the 4294967296 B" err
        # the variables take the whole '--vars' capacity after the dats
        printf 'dat big = i8[4294967000]\nfunc main(){\n    exit(0)\n}\n' >gen-range.baz
        expect_exit_1 baz_at "$target" gen-range.baz >gen.s 2>err
        grep -Fq "gen-range.baz:1:5: data and variables use 4295032544 B, which reaches" err
    done
    # x86_64 too: each is below 2^47, their sum is not
    printf 'dat d = i8[0x7fff00000000]\nfunc main() {\n    exit(0)\n}\n' \
        >gen-range.baz
    expect_exit_1 "$BIN" --vars=0x7ffffffffff0 gen-range.baz >gen.s 2>err
    grep -Fq "gen-range.baz:1:5: data and variables use 281470681743344 B, which reaches" err
    rm -f gen-range.baz gen-rv32i.bin
    echo ok
}

# compiled only, the program would zero 3 GB
CLI_FAR_INDEXED_ELEMENT() {
    echo -n "cli x86_64 indexed element past the displacement range: "
    printf 'func main() {\n    var big = i8[3000000000]\n    var a = i64[4]\n' >gen-far.baz
    printf '    var i = 2\n    a[i] = 7\n    exit(a[i])\n}\n' >>gen-far.baz
    "$BIN" --vars=0x100000000 gen-far.baz >gen.s 2>err
    [[ ! -s err ]]
    # a register sums the base and the offset of 'a', then the index
    grep -Eq '^ +lea r[0-9]+, \[r[0-9]+ \+ r[0-9]+ \* 8\]$' gen.s
    rm -f gen-far.baz
    echo ok
}

# compiled only, the program would zero 2.2 GB; an array longer than the
# signed range of the default type tests the sign of its index, which the
# unsigned upper compare does not cover (rv32i)
CLI_LONG_ARRAY_LOWER_BOUND() {
    echo -n "cli rv32i lower bound of an array past 2^31 elements: "
    printf 'func main() {\n    var big = i8[2200000000]\n    var i = 2\n' >gen-long.baz
    printf '    big[i] = 1\n    exit(big[i])\n}\n' >>gen-long.baz
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --vars=0x90000000 \
        --checks=upper,lower gen-long.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Eq '^ +blt t0, zero, baz_bounds_panic$' gen.s
    # the bound is exact: every negative index fails the unsigned compare of
    # an array of 2^31 - 1 elements, the index -2^31 passes that of 2^31 + 1
    sed -i 's/2200000000/2147483647/' gen-long.baz
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --vars=0x90000000 \
        --checks=upper,lower gen-long.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq 't0 lower bound covered by the unsigned upper bound' gen.s
    grep -Eq '^ +blt t0, zero, baz_bounds_panic$' gen.s && fail
    sed -i 's/2147483647/2147483649/' gen-long.baz
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --vars=0x90000000 \
        --checks=upper,lower gen-long.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Eq '^ +blt t0, zero, baz_bounds_panic$' gen.s
    rm -f gen-long.baz
    echo ok
}

# compiled only, the fpga cannot hold the arrays; under '--checks=upper' alone
# an index of 32 bits is not compared with a count of 2^31 or more, and the
# end of a range is compared unsigned after a negative high word passed
CLI_LONG_ARRAY_UPPER_BOUND() {
    echo -n "cli rv32i upper bound of an array past 2^31 elements: "
    local fpga=(--target=rv32i-fpga --bin=gen-rv32i.bin --memory=0xfffff000
        --stack=48 --vars=0x80000040 --checks=upper)
    "$BIN" "${fpga[@]}" 2202.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c 'upper bound: a 32-bit index is below the count$' gen.s) -eq 4 ]]
    grep -Eq '^ +li t[0-9], 2147483664$' gen.s && fail
    "$BIN" "${fpga[@]}" 2203.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -Ec '^ +blt t[0-9], zero, 2f$' gen.s) -eq 3 ]]
    [[ $(grep -Ec '^ +bgeu t[0-9], t[0-9], 2f$' gen.s) -eq 3 ]]
    rm -f gen-rv32i.bin
    echo ok
}

# compiled only, the program never ends; a 'main' that neither ends nor
# exits has no exit routine
CLI_NO_EXIT_ROUTINE() {
    echo -n "cli no exit routine: "
    printf 'func main() {\n    loop {\n    }\n}\n' >gen-loop.baz
    local target
    for target in x86_64 rv32i; do
        baz_at "$target" gen-loop.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Fq baz_exit gen.s && fail
    done
    rm -f gen-loop.baz
    echo ok
}

CLI_FPGA_MEMORY() {
    echo -n "cli rv32i-fpga memory size: "
    local memory_size=$((0x800000)) stack_size=$((0x10000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    # the variables follow the image at the next 16 byte boundary
    local image_size vars_fit
    image_size=$(stat -c %s gen-rv32i.bin)
    vars_fit=$((memory_size - stack_size - (image_size + 15) / 16 * 16))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$vars_fit 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    rm -f gen-rv32i.bin
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=$((vars_fit + 16)) 0418.baz >gen.s 2>err
    [[ ! -s gen.s && ! -e gen-rv32i.bin ]]
    grep -Fq "exceeds the $memory_size B of device memory" err
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=$((memory_size + 16)) 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: $((memory_size + 16)) B exceeds the $memory_size B of device memory" err
    grep -Fq "use --help for usage information" err
    # a memory smaller than the default stack names '--memory', the stack was
    # not given
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=0x1000 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --memory: 4096 B is smaller than the default stack of 65536 B, give a smaller --stack" err
    # it is an option error, reported before the source is read
    "$BIN" --target=rv32i-fpga --memory=0x1000 missing.baz >gen.s 2>err || true
    grep -Fq "error: invalid --memory: 4096 B is smaller than the default stack" err
    # a stack larger than the memory is rejected before a stack check of a
    # 'noinline' call is generated with it
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --checks=stack --stack=$((memory_size + 16)) 0981.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: $((memory_size + 16)) B exceeds the $memory_size B of device memory" err
    # the options are checked before the source, so an error in the source
    # does not hide a stack below what 'main' may use
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=16 0212.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: 16 B is smaller than the 48 B" err
    grep -Fq "'nope' not found" err && fail
    # a size beyond the range of a number is out of range, not a non-number
    expect_exit_1 "$BIN" --target=rv32i-fpga --stack=99999999999999999999999 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: '99999999999999999999999' is out of range" err
    # a smaller memory moves the end of the stack and the limit of the vars
    local small_size=$((0x40000))
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "load stack pointer to 0x4:0000" gen.s
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=$small_size --vars=$((small_size)) 0418.baz >gen.s 2>err
    grep -Fq "exceeds the $small_size B of device memory" err
    local bad
    for bad in 0 100 0x100000000 0xfffff800 abc; do
        expect_exit_1 "$BIN" --target=rv32i-fpga --memory=$bad 0418.baz >gen.s 2>err
        [[ ! -s gen.s ]]
    done
    # the target rejects a memory that reaches its devices
    "$BIN" --target=rv32i-fpga --memory=0x100000000 0418.baz >gen.s 2>err || true
    grep -Fq "error: invalid --memory: 4294967296 B exceeds the 4294963200 B below the device addresses" err
    # the largest memory below the devices is accepted
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=0xfffff000 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    echo "ok (image $image_size B, vars up to $vars_fit B)"
}

CLI_NOINLINE_REPORT() {
    echo -n "cli noinline report: "
    local target
    for target in x86_64 rv32i; do
        # 0751.baz: 'sum' has an instance per array length, 'length_of' a body
        # for each of its two calls
        baz_at "$target" 0751.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Eq '^[;#] +sum: 2 bodies, 3 calls, [0-9]+ instructions$' gen.s
        grep -Eq '^[;#] +length_of: 2 bodies, 2 calls, [0-9]+ instructions, no reuse$' gen.s
        [[ $(grep -Ec '^[[:space:]]*call func\.sum\.\.' gen.s) -eq 3 ]]
        # the report comes before the optimization counts
        [[ $(grep -n 'noinline functions:' gen.s | cut -d: -f1) -lt $(grep -n 'removed jumps to next code' gen.s | cut -d: -f1) ]]
    done
    # 0753.baz: one call of 'box.total'
    "$BIN" 0753.baz >gen.s 2>err
    grep -Eq '^; +box.total: 1 body, 1 call, [0-9]+ instructions, no reuse$' gen.s
    rm -f gen-rv32i.bin
    echo ok
}

CLI_STACK_HANDLER() {
    echo -n "cli --checks=stack handler: "
    # an operating system stops a stack overflow, no handler is written
    local target
    for target in x86_64 rv32i; do
        baz_at "$target" --checks=stack 0981.baz >gen.s 2>err
        [[ ! -s err ]]
        grep -Fq "stack overflow handler" gen.s && fail
    done
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --checks=stack 0981.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "stack overflow handler (--checks=stack)" gen.s
    rm -f gen-rv32i.bin
    echo ok
}

CLI_VARS_RANGE() {
    echo -n "cli --vars past the target addresses: "
    expect_exit_1 "$BIN" --vars=0x100000010 --target=rv32i 0418.baz >gen.s 2>err
    grep -Fq "invalid --vars: 4294967312 B reaches the 4294967296 B that the target addresses" err
    grep -Fq "use --help for usage information" err
    # the variables stay below the end, so the whole address space is too
    # large too
    "$BIN" --vars=0x100000000 --target=rv32i 0418.baz >gen.s 2>err || true
    grep -Fq "invalid --vars: 4294967296 B reaches the 4294967296 B that the target addresses" err
    echo ok
}

CLI_QEMU_STACK() {
    echo -n "cli rv32i-qemu stack size: "
    expect_exit_1 "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x100000010 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: 4294967312 B exceeds the 132120576 B of device memory" err
    grep -Fq "use --help for usage information" err
    # the address space, one more than the largest 32-bit immediate
    expect_exit_1 "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x100000000 0418.baz >gen.s 2>err
    grep -Fq "error: invalid --stack: 4294967296 B exceeds the 132120576 B of device memory" err
    grep -Fq "use --help for usage information" err
    # the 'virt' machine has 128 MiB, 126 MiB of it for the image
    "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x7d00000 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    expect_exit_1 "$BIN" --target=rv32i-qemu --bin=gen-rv32i.bin --stack=0x7e00000 0418.baz >gen.s 2>err
    grep -Fq "the stack 132120576 B, which exceeds the 132120576 B of device memory" err
    rm -f gen-rv32i.bin
    echo ok
}

CLI_SMALL_STACK() {
    echo -n "cli bare-metal stack below what 'main' may use: "
    # 'main' has no stack check, a helper call and a failure use 48 B
    local target
    for target in rv32i-qemu rv32i-fpga; do
        expect_exit_1 baz_at "$target" --stack=32 0418.baz >gen.s 2>err
        grep -Fq "error: invalid --stack: 32 B is smaller than the 48 B that 'main' may use without a stack check" err
        grep -Fq "use --help for usage information" err
        baz_at "$target" --stack=48 0418.baz >gen.s 2>err
        [[ ! -s err ]]
    done
    rm -f gen-rv32i.bin
    echo ok
}

# the footer counts the scratch registers of the kept version: 1213.baz keeps
# the version that holds fewer by default and the other one with 'last'
CLI_VERSIONS() {
    echo -n "cli --versions: "
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 1213.baz >gen.s 2>err
    grep -Fxq "# max scratch registers in use: 15" gen.s
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --versions=last 1213.baz >gen.s 2>err
    grep -Fxq "# max scratch registers in use: 29" gen.s
    # 1661.baz: a call in a value stored to memory compiles in place without a
    # temporary, the version through a scratch register holds one across the
    # call
    "$BIN" --versions=first 1661.baz >gen.s 2>err
    grep -Fxq ";           max body vars size: 16 B" gen.s
    grep -Fxq ";            max scopes in use: 3" gen.s
    "$BIN" --versions=last 1661.baz >gen.s 2>err
    grep -Fxq ";           max body vars size: 24 B" gen.s
    "$BIN" --vars=16 1661.baz >gen.s 2>err
    [[ ! -s err ]]
    # 0106.baz: choices nested in a version take the first unless 'nested'
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 0106.baz >gen.s 2>err
    grep -Fq "kept the first of two versions, inside a version" gen.s
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --versions=nested 0106.baz >gen.s 2>err
    [[ ! -s err ]]
    grep -Fq "inside a version" gen.s && fail
    echo ok
}

CLI_PADDING_ZERO() {
    echo -n "cli padding in array elements zeroed as one instance: "
    local target
    for target in x86_64 rv32i-fpga; do
        # 1269.baz: 4000 elements with 3 B of padding each, one zero of the
        # instance instead of a store per gap
        baz_at "$target" 1269.baz >gen.s 2>err
        [[ ! -s err ]]
        [[ $(grep -c "zero instance for its padding: 32000 B" gen.s) -eq 1 ]]
        [[ $(grep -c "zero padding" gen.s) -eq 0 ]]
        [[ $(wc -l <gen.s) -lt 1000 ]]
    done
    echo ok
}

CLI_PADDING_ZERO_BY_TARGET() {
    echo -n "cli padding of six gaps zeroed as the target emits it smaller: "
    # 1275.baz: x86_64 zeroes the 48 B instance, rv32i each of its 6 gaps
    "$BIN" --target=x86_64 1275.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c "zero instance for its padding: 48 B" gen.s) -eq 1 ]]
    [[ $(grep -c "zero padding" gen.s) -eq 0 ]]
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 1275.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c "zero instance for its padding" gen.s) -eq 0 ]]
    [[ $(grep -c "zero padding: 3 B" gen.s) -eq 6 ]]
    echo ok
}

CLI_SHARED_STRING_CONSTANT() {
    echo -n "cli strings with the same bytes share one constant: "
    # 1636.baz: '"hello..."' and '"\x68ello..."' are one label on each target
    "$BIN" --target=x86_64 1636.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c '^str\.' gen.s) -eq 1 ]]
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin 1636.baz >gen.s 2>err
    [[ ! -s err ]]
    [[ $(grep -c '^str\.' gen.s) -eq 1 ]]
    echo ok
}

CLI_OPTION_MESSAGES() {
    echo -n "cli option errors name their option: "
    local arg
    for arg in --vars=0 --stack=x --memory=100 --target=x --checks=x --report=x --versions=x; do
        expect_exit_1 "$BIN" "$arg" 0015.baz >/dev/null 2>err
        grep -Fq "error: invalid ${arg%%=*}: " err
    done
    # each names what it lists
    "$BIN" --checks=x 0015.baz >/dev/null 2>err || true
    grep -Fq "supported checks are: " err
    grep -Fq ", noub, each with a '+' or '-' prefix" err
    "$BIN" --target=x 0015.baz >/dev/null 2>err || true
    grep -Fq "', supported targets are: x86_64, rv32i" err
    "$BIN" --report=x 0015.baz >/dev/null 2>err || true
    grep -Fq "', supported reports are: registers" err
    "$BIN" --versions=x 0015.baz >/dev/null 2>err || true
    grep -Fq "', supported values are: smaller, nested, first, last" err
    "$BIN" --vars=17 0015.baz >/dev/null 2>err || true
    grep -Fq "error: invalid --vars: '17' is not a multiple of 16" err
    "$BIN" --bin= 0015.baz >/dev/null 2>err || true
    grep -Fq "error: invalid --bin: empty file name" err
    "$BIN" -x 0015.baz >/dev/null 2>err || true
    grep -Fq "error: unknown option: '-x'" err
    "$BIN" 0015.baz 0418.baz >/dev/null 2>err || true
    grep -Fq "error: more than one source file: '0015.baz' and '0418.baz'" err
    echo ok
}

# the options only some targets read are rejected by the others, also when
# '--target' comes after them
CLI_TARGET_OPTIONS() {
    echo -n "cli options a target does not read: "
    local -a rejected=(
        "--stack=0x10000 0418.baz"
        "--target=rv32i --stack=0x10000 0418.baz"
        "--memory=0x100000 --target=rv32i-qemu 0418.baz"
        "--target=x86_64 --memory=0x100000 0418.baz"
        "--target=rv32i-fpga --sdcard=other.img 0418.baz"
        "--sdcard=other.img --target=rv32i-qemu 0418.baz"
    )
    local args
    for args in "${rejected[@]}"; do
        # shellcheck disable=SC2086
        expect_exit_1 "$BIN" --bin=gen-rv32i.bin $args >gen.s 2>err
        grep -Eq "^error: invalid --(stack|memory|sdcard): target '[^']+' does not read it, the targets that do are: " err
    done
    grep -Fq "error: invalid --sdcard: target 'rv32i-qemu' does not read it, the targets that do are: x86_64, rv32i" err
    "$BIN" --stack=0x10000 0418.baz 2>err >/dev/null || true
    grep -Fq "the targets that do are: rv32i-qemu, rv32i-fpga" err
    "$BIN" --memory=0x100000 0418.baz 2>err >/dev/null || true
    grep -Fq "the targets that do are: rv32i-fpga" err
    # the targets that read them accept them, 1048 calls the sector functions
    "$BIN" --sdcard=other.img 1048.baz >gen.s 2>err
    grep -Fq "other.img" gen.s
    "$BIN" --target=rv32i --bin=gen-rv32i.bin --sdcard=other.img 1048.baz >gen.s 2>err
    grep -Fq "other.img" gen.s
    "$BIN" --stack=0x10000 --target=rv32i-qemu --bin=gen-rv32i.bin 0418.baz >gen.s 2>err
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --stack=0x10000 --memory=0x100000 0418.baz >gen.s 2>err
    [[ ! -s err ]]
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
CLI --vars=0x8000000000000000 1 0418.baz
CLI --vars=0x800000000000 1 0418.baz
CLI --vars=0x7ffffffffff0 0 0418.baz
CLI --stack=65536 0 --help
CLI --stack=0x10000 0 --help
CLI --stack= 1 --help
CLI --stack=0 1 --help
CLI --stack=17 1 --help
CLI --stack=16junk 1 --help
CLI --no-reproduce 1 --help
CLI --checks=frame 0 0015.baz
CLI --checks=upper,lower,line,frame 0 0015.baz
CLI --checks=noub 0 0015.baz
CLI --checks=+division 0 0015.baz
CLI --checks=noub,-division 0 0015.baz
CLI --checks=-noub 0 0015.baz
CLI --checks=+ 1 0015.baz
# the option is accepted, the compile is rejected by the source
CLI --checks=upper 1 0930.baz
grep -Fq "may share storage with the result destination" err
CLI --checks=noub 1 0930.baz
grep -Fq "may share storage with the result destination" err
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
CLI 0015.baz 1 0418.baz
CLI 0015.baz 0 --nopt
CLI_TARGETS
CLI_BINARY_NAME
CLI_REPRODUCE_SOURCE
CLI_JUMP_OPTIMIZATIONS
CLI_RELOADS
CLI_RETESTS
CLI_SHARED_BASE_COPY
CLI_ROUTINE_INDENT
CLI_KEYWORD_NAMES() {
    echo -n "cli keywords as names: "
    # every keyword of 'token.hpp' is rejected as the name of a variable
    local keyword
    for keyword in func noinline mut type dat var let foo loop if else \
        continue break return self and or not true false include; do
        printf 'func main() {\n    var %s = 1\n}\n' "$keyword" >gen-keyword.baz
        expect_exit_1 "$BIN" gen-keyword.baz >out 2>&1
        grep -Fq "gen-keyword.baz:2:9: '$keyword' is a keyword" out
    done
    rm -f gen-keyword.baz
    echo ok
}

CLI_REGISTER_REPORT_COUNTS() {
    echo -n "cli --report=registers counts: "
    "$BIN" --report=registers 2474.baz >out 2>err
    [[ ! -s err ]]
    # the first of the equal peaks of lines 15 and 16
    grep -Fxq ";           r15 15:18" out
    grep -Fxq ";           r14 15:23" out
    # a noinline body counts what it holds, not the frame base; an inlined
    # callee counts its calls
    grep -Fxq ";     2      3  g (noinline body)" out
    grep -Fxq ";     1      2  f" out
    # the first of the calls that save one register
    grep -Fxq ";       1      3  g 17:17" out
    # 2476.baz: the footer has the most scratch registers, not the last count
    "$BIN" 2476.baz >out 2>err
    grep -Fxq "; max scratch registers in use: 2" out
    # 2475.baz: the peak of 0 registers is in 'main' alone
    "$BIN" --report=registers 2475.baz >out 2>err
    grep -Eq '^; +0  main$' out
    grep -Fq "h called at" out && fail
    # 0753.baz: the callees and the saved registers in descending order, equal
    # counts by name
    "$BIN" --report=registers 0753.baz >out 2>err
    grep -A1 -Fx ";   own  calls  callee" out | grep -Fxq ";     6      1  box.total (noinline body)"
    grep -A2 -Fx ";   saved  calls  callee, most saved at" out | tail -1 |
        grep -Fxq ";       0      2  box.take 63:12"
    echo ok
}

# a memory of the default stack has the error of what exceeds it, a smaller
# memory with a smaller stack given is accepted
CLI_MEMORY_DEFAULT_STACK() {
    echo -n "cli rv32i-fpga memory and the default stack: "
    expect_exit_1 "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --memory=0x10000 0418.baz >gen.s 2>err
    grep -Fq "and the stack 65536 B, which exceeds the 65536 B of device memory" err
    "$BIN" --target=rv32i-fpga --bin=gen-rv32i.bin --vars=0x1000 --memory=0x8000 --stack=0x1000 0418.baz >gen.s 2>err
    [[ ! -s err ]]
    rm -f gen-rv32i.bin
    echo ok
}

CLI_CHECKS_NOUB
CLI_CHECKS_LIST
CLI_FILE_ERRORS
CLI_FPGA_MEMORY
CLI_QEMU_STACK
CLI_SMALL_STACK
CLI_OPTION_MESSAGES
CLI_TARGET_OPTIONS
CLI_VARS_RANGE
CLI_STACK_HANDLER
CLI_NOINLINE_REPORT
CLI_REGISTER_REPORT
CLI_REGISTER_REPORT_COUNTS
CLI_MEMORY_DEFAULT_STACK
CLI_ADDRESS_RANGE
CLI_FAR_INDEXED_ELEMENT
CLI_LONG_ARRAY_LOWER_BOUND
CLI_LONG_ARRAY_UPPER_BOUND
CLI_NO_EXIT_ROUTINE
CLI_SOURCE_LINE_ENDS
CLI_ERROR_LINE_BYTES
CLI_UNINSTANTIATED_GENERICS
CLI_TYPE_LAYOUT_COMMENT
CLI_ZERO_COMMENTS
CLI_COMMENT_LOCATIONS
CLI_INSTANCE_COMMENT
CLI_CONTROL_BYTE_FILE_NAME
CLI_INCLUDE_THROUGH_LINK
CLI_VERSIONS
CLI_PADDING_ZERO
CLI_PADDING_ZERO_BY_TARGET
CLI_SHARED_STRING_CONSTANT
CLI_KEYWORD_NAMES

rm -f gen.s diff.baz out err gen-rv32i.bin
