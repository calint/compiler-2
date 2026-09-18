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

  /* dedicated downward-growing stack arena used by rsp-relative slots */
  .bss.stack (NOLOAD) : ALIGN(16)
  {
    *(.bss.stack)
    __stack_section_end = .;
  } :data

  /* initialized global dat payload; rsp starts at dat.end */
  .data :
  {
    __dat_section_start = .;
    *(.data .data.*)
  } :data

  ASSERT(__stack_section_end == __dat_section_start,
         "baz.ld: .bss.stack and .data must be adjacent")

  /* other zero-initialized writable storage (non-stack) */
  .bss (NOLOAD) : ALIGN(16)
  {
    *(.bss .bss.*)
    *(COMMON)
  } :data
}
