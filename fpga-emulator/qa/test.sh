#!/bin/sh
set -e
cd "$(dirname "$0")"

echo " * build rv32i for tests"

CXX="g++ -std=c++23 -O3 -fno-rtti -fno-exceptions -Wfatal-errors -Werror -Wall -Wextra -Wpedantic \
    -Wconversion -Wsign-conversion -Wswitch-default -Wimplicit-fallthrough \
    -Wshadow -Wlogical-op -Wnon-virtual-dtor -Wcast-align -Woverloaded-virtual \
    -Wduplicated-cond -Wduplicated-branches -Wnull-dereference -Wuseless-cast \
    -Wdouble-promotion -Wmisleading-indentation -Wformat=2"

CMD="$CXX -o osqa-test main.cpp"
#echo
#echo $CMD
#echo
$CMD

ls -l --color osqa-test

#
# compile test cases
#
CC=riscv64-elf-g++
OBJCOPY=riscv64-elf-objcopy
OBJDUMP=riscv64-elf-objdump

SRC=ram.S
BIN=${SRC%.*}

# -mstrict-align \
$CC -std=c++23 \
    -march=rv32i \
    -mabi=ilp32 \
    -O0 \
    -ffreestanding \
    -nostartfiles \
    -nostdlib \
    -fno-pic \
    -Wfatal-errors \
    -Wall -Wextra -pedantic \
    -Wl,-Ttext=0x0 \
    -Wl,--no-relax \
    -fno-toplevel-reorder \
    -o $BIN $SRC

$OBJCOPY $BIN --output-target=binary $BIN.bin
$OBJDUMP --disassembler-options=numeric,no-aliases --disassemble --reloc $BIN >$BIN.lst

rm $BIN

./osqa-test

echo " * build sd card test"

$CXX -o sdcard-test sdcard.cpp

# sparse images, sized above 4 GB without using the space
IMAGE="$(mktemp)"
FIRMWARE_IMAGE="$(mktemp)"
trap 'rm -f "$IMAGE" "$FIRMWARE_IMAGE"' EXIT
./sdcard-test "$IMAGE"

echo " * run sd card firmware test in the emulator"

../make.sh >/dev/null

SRC=sdcard-firmware.cpp
BIN=${SRC%.*}

# no loop is turned into a call of 'memset' or 'memcpy', there is no library
$CC -std=c++23 \
    -march=rv32i \
    -mabi=ilp32 \
    -O2 \
    -ffreestanding \
    -fno-tree-loop-distribute-patterns \
    -nostartfiles \
    -nostdlib \
    -fno-pic \
    -Wfatal-errors \
    -Wall -Wextra -pedantic \
    -Wl,-Ttext=0x0 \
    -Wl,--no-relax \
    -fno-toplevel-reorder \
    -o $BIN $SRC

$OBJCOPY $BIN --output-target=binary $BIN.bin
$OBJDUMP --disassembler-options=numeric,no-aliases --disassemble --reloc $BIN >$BIN.lst

rm $BIN

# 5 GB and one sector
truncate -s $((5 * 1024 * 1024 * 1024 + 512)) "$FIRMWARE_IMAGE"

../osqa $BIN.bin "$FIRMWARE_IMAGE" </dev/null || {
    echo "sd card firmware test $? FAILED"
    exit 1
}

# the sector above 4 GB is in the image: bytes 3, 10, 17 and 24 of the
# pattern 'seed + i * 7' with seed 3
BYTES="$(od -An -tu1 -j $((8388608 * 512)) -N4 "$FIRMWARE_IMAGE" | tr -s ' ')"
if [ "$BYTES" != " 3 10 17 24" ]; then
    echo "sd card image check FAILED: '$BYTES'"
    exit 1
fi

echo " * run misaligned access and decoding tests in the emulator"

# CASE=N assembles the case N of 'misaligned.S', which must stop with the exit
# code and print the message
misaligned() {
    $CC -march=rv32i \
        -mabi=ilp32 \
        -nostartfiles \
        -nostdlib \
        -Wl,-Ttext=0x0 \
        -Wl,--no-relax \
        -DCASE=$1 \
        -o misaligned misaligned.S

    $OBJCOPY misaligned --output-target=binary misaligned.bin
    rm misaligned

    STATUS=0
    ../osqa misaligned.bin "$IMAGE" </dev/null >misaligned.err 2>&1 || STATUS=$?
    if [ "$STATUS" != "$2" ] || ! grep -qxF "$3" misaligned.err; then
        echo "misaligned test $1 FAILED: exit code $STATUS"
        cat misaligned.err
        exit 1
    fi
}

misaligned 1 199 "misaligned fetch at pc 0x0000002e address 0x0000002e"
misaligned 2 200 "misaligned lh at pc 0x00000028 address 0x00001001"
misaligned 3 201 "misaligned lhu at pc 0x00000028 address 0x00001003"
misaligned 4 202 "misaligned lw at pc 0x00000028 address 0x00001002"
misaligned 5 203 "misaligned sh at pc 0x00000028 address 0x00001001"
misaligned 6 204 "misaligned sw at pc 0x00000028 address 0x00001002"
misaligned 7 199 "misaligned fetch at pc 0x0000002d address 0x0000002d"
misaligned 8 12 "CPU error: 12"
misaligned 9 11 "CPU error: 11"
misaligned 10 13 "CPU error: 13"
misaligned 11 14 "CPU error: 14"

rm misaligned.bin misaligned.err

echo "test: PASSED"
