# osqa emulator

Emulates the FPGA program.

## usage

`./make.sh` to build the emulator

`./osqa ../prog-rv32i-fpga.bin sdcard.img` to run an image that
`baz --target=rv32i-fpga` wrote, with an SD card image;
`../run-rv32i-fpga.sh` does the same with a temporary card.

## todo

```
[ ] record the maximum used stack space during a run
[ ] building the immediate values can be done with 1 AND, 1 SHIFT per section
    using a hardcoded mask instead of current 3 SHIFT, 1 AND, 2 ADD, 1 SUB
[ ] in debug mode print the values of used registers, immediate values and result
```
