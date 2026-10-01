#!/bin/sh
set -e

# resolve before changing directory since the argument is relative to the caller
SOURCE_FILE=""
if [ -n "$1" ]; then
    SOURCE_FILE=$(realpath "$1")
fi

cd $(dirname "$0")

# override configuration
. ./fpga-config-20k.sh

cd ..

if [ -z "$SOURCE_FILE" ]; then
    SOURCE_FILE="etc/roome/roome.baz"
fi

echo
echo "building firmware from '$SOURCE_FILE'"

./baz --target=rv32i-fpga --vars=131072 --checks=noub,line --bin="$FIRMWARE_FILE" "$SOURCE_FILE" >"${SOURCE_FILE%.baz}.s"

# check result
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo
    echo -e "\e[31mbuild failed. firmware file '$FIRMWARE_FILE' not created.\e[0m"
    exit 1
fi

# check file size
FILE_SIZE=$(stat -c %s "$FIRMWARE_FILE")

echo -e "\e[32m"
echo "file: $FIRMWARE_FILE"
echo "size: $FILE_SIZE B"
echo " max: $FIRMWARE_FILE_MAX_SIZE_BYTES B"
echo -e "\e[0m"

if [ "$FILE_SIZE" -gt "$FIRMWARE_FILE_MAX_SIZE_BYTES" ]; then
    echo -e "\e[31mfirmware size exceeds allocated flash storage.\e[0m"
    exit 1
fi

echo "flashing '$FIRMWARE_FILE' to '$BOARD_NAME' at offset $FIRMWARE_FLASH_OFFSET"
echo

openFPGALoader --offset $FIRMWARE_FLASH_OFFSET --external-flash "$FIRMWARE_FILE"
