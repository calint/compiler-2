/* lays out 'gen.s' like the compiler's flat image, the variables are not part of it */
SECTIONS {
    . = 0;
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss (NOLOAD) : { *(.bss) }
}
