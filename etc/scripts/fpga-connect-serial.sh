#!/bin/sh
set -e
cd $(dirname "$0")

picocom --baud 115200 --noreset --flow n --parity n --databits 8 /dev/ttyUSB1
