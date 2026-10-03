#!/bin/sh
set -eu

print_usage() {
    echo "usage: fpga-flash.sh <20k|9k> [source file]"
    echo
    echo "  20k | 9k      required, the board to flash (tangnano20k or tangnano9k)"
    echo "  source file   optional, defaults to etc/roome/roome.baz"
}

BOARD="${1:-}"

if [ "$BOARD" = "-h" ] || [ "$BOARD" = "--help" ]; then
    print_usage
    exit 0
fi

if [ "$BOARD" != "20k" ] && [ "$BOARD" != "9k" ]; then
    print_usage >&2
    exit 1
fi

# resolve before changing directory since the argument is relative to the caller
SOURCE_FILE=""
if [ -n "${2:-}" ]; then
    SOURCE_FILE=$(realpath "$2")
fi

cd "$(dirname "$0")"

# override configuration
. "./fpga-config-$BOARD.sh"

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
    printf '\033[31mbuild failed. firmware file '"'%s'"' not created.\033[0m\n' "$FIRMWARE_FILE"
    exit 1
fi

# check file size
FILE_SIZE=$(stat -c %s "$FIRMWARE_FILE")

printf '\033[32m\n'
echo "file: $FIRMWARE_FILE"
echo "size: $FILE_SIZE B"
echo " max: $FIRMWARE_FILE_MAX_SIZE_BYTES B"
printf '\033[0m\n'

if [ "$FILE_SIZE" -gt "$FIRMWARE_FILE_MAX_SIZE_BYTES" ]; then
    printf '\033[31mfirmware size exceeds allocated flash storage.\033[0m\n'
    exit 1
fi

echo "flashing '$FIRMWARE_FILE' to '$BOARD_NAME' at offset $FIRMWARE_FLASH_OFFSET"
echo

openFPGALoader --offset "$FIRMWARE_FLASH_OFFSET" --external-flash "$FIRMWARE_FILE"
