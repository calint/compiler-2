#!/bin/sh
set -eu

# resolve before changing directory since the argument is relative to the caller
if [ $# -gt 0 ]; then
    set -- "$(realpath "$1")"
fi

cd "$(dirname "$0")"

exec ./fpga-flash.sh 9k "$@"
