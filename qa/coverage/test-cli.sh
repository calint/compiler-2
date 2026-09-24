#!/bin/bash
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
    [[ "$optimized" == *';          optimization pass 1: 2'* ]]
    [[ "$optimized" == *';          optimization pass 2: 1'* ]]
    [[ "$raw" != *'optimization pass'* ]]
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
    echo "ok (pass 1: 2, pass 2: 1; jumps $raw_count -> $optimized_count; both exit 0)"
}

CLI --vars=65536 0 --help
CLI --vars=0x10000 0 --help
CLI --vars= 1 --help
CLI --vars=0 1 --help
CLI --vars=-16 1 --help
CLI --vars=17 1 --help
CLI --vars=16junk 1 --help
CLI --vars=18446744073709551616 1 --help
CLI --stack=65536 1 --help
CLI --no-reproduce 1 --help
CLI --checks=frame 0 015.baz
CLI --checks=alias 0 015.baz
CLI --checks=upper,lower,line,frame,alias 0 015.baz
CLI --checks=unknown 1 --help
CLI --target=x86_64 0 --help
CLI --target=rv32i 0 --help
CLI --target= 1 --help
CLI --target=unknown 1 --help
CLI_TARGETS
CLI_REPRODUCE_SOURCE
CLI_JUMP_OPTIMIZATIONS

rm -f gen.s diff.baz out err
