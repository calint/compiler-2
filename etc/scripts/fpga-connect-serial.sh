#!/bin/sh
set -e
cd $(dirname "$0")

# the board sends bare line feeds, so the terminal adds the carriage return
picocom --baud 115200 --noreset --flow n --parity n --databits 8 --imap lfcrlf /dev/ttyUSB1
