#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BIN="../../baz"

CLI() {
    echo -n "cli $1: "
    set +e
    LLVM_PROFILE_FILE="cli-$1.profraw" $BIN "$1" "${@:3}" >/dev/null 2>err
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
    LLVM_PROFILE_FILE="cli-default.profraw" $BIN t15.baz >gen.s 2>err
    [[ ! -e diff.baz ]]
    cp t1.baz diff.baz
    LLVM_PROFILE_FILE="cli-default-nopt.profraw" $BIN t15.baz --nopt >out 2>err
    cmp -s diff.baz t1.baz
    LLVM_PROFILE_FILE="cli-reproduce.profraw" $BIN t15.baz --reproduce-source >out 2>err
    cmp -s diff.baz t15.baz
    cmp -s gen.s out
    rm -f diff.baz
    LLVM_PROFILE_FILE="cli-reproduce-nopt.profraw" $BIN --reproduce-source t15.baz --nopt >out 2>err
    cmp -s diff.baz t15.baz
    echo ok
}

CLI_TARGETS() {
    echo -n "cli target selection: "
    $BIN t15.baz >gen.s 2>err
    $BIN --target=x86_64 t15.baz >out 2>err
    cmp -s gen.s out
    $BIN --nopt t15.baz >gen.s 2>err
    $BIN --target=x86_64 --nopt t15.baz >out 2>err
    cmp -s gen.s out
    local exit_code=0
    $BIN --target=rv32i t15.baz >out 2>err || exit_code=$?
    [[ $exit_code -eq 1 ]]
    [[ ! -s out ]]
    grep -Fxq "todo" err
    grep -Fxq "panic: RV32I backend not implemented" err
    echo ok
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
CLI --target=x86_64 0 --help
CLI --target=rv32i 0 --help
CLI --target= 1 --help
CLI --target=unknown 1 --help
CLI_TARGETS
CLI_REPRODUCE_SOURCE

rm -f gen.s diff.baz out err