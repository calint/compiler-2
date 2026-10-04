#!/bin/sh
set -eu

# resolve before changing directory since the argument is relative to the caller
if [ $# -gt 0 ] && [ -n "$1" ]; then
    FIRST=$(realpath "$1")
    shift
    set -- "$FIRST" "$@"
fi

cd "$(dirname "$0")"

exec ./fpga-flash.sh 9k "$@"
