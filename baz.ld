ENTRY(_start)

PHDRS
{
  /* executable/read-only load segment */
  text PT_LOAD FLAGS(5); /* R + X */
  /* writable load segment */
  data PT_LOAD FLAGS(6); /* R + W */
}

SECTIONS
{
  /* conventional x86-64 ELF image base for this static layout */
  . = 0x400000 + SIZEOF_HEADERS;

  /* program code */
  .text : ALIGN(16)
  {
    *(.text .text.*)
  } :text

  /* read-only constants */
  .rodata : ALIGN(16)
  {
    *(.rodata .rodata.*)
  } :text

  /* start writable segment on a fresh page to avoid overlap with text */
  . = ALIGN(0x1000);

  /* initialized global dat payload; rbp points to dat */
  .data :
  {
    __dat_section_start = .;
    *(.data .data.*)
    __dat_section_end = .;
  } :data

  /* upward-growing variable arena, without file-backed storage */
  .bss.stack (NOLOAD) : ALIGN(16)
  {
    __stack_section_start = .;
    *(.bss.stack)
  } :data

  ASSERT(__stack_section_start == ALIGN(__dat_section_end, 16),
         "baz.ld: .bss.stack must follow .data with 16-byte alignment")

  /* other zero-initialized writable storage (non-stack) */
  .bss (NOLOAD) : ALIGN(16)
  {
    *(.bss .bss.*)
    *(COMMON)
  } :data
}
