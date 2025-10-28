OUTPUT_ARCH(aarch64)

ENTRY(_start)

MEMORY
{
    ram (rwx)  : ORIGIN = 0x890400000, LENGTH = 0x10000
}

SECTIONS
{
  .text ORIGIN(ram) : {
    *(.text.boot)
    *(.text)
    *(.text.*)
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
  } > ram

  __text_end = ABSOLUTE(.);

  .data : AT (__text_end) {
    __data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    *(.sdata)
    *(.sdata.*)
    . = ALIGN(4);
    __data_end = ABSOLUTE(.);
  } > ram

  _stack = ORIGIN(ram) + LENGTH(ram) - 0x20;

       /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.debug*) *(.hash*) *(.rela*) }
       /DISCARD/ : { *(.dynsym) }
       /DISCARD/ : { *(.dynstr*) }
       /DISCARD/ : { *(.dynamic*) }
       /DISCARD/ : { *(.plt*) }
       /DISCARD/ : { *(.interp*) }
       /DISCARD/ : { *(.gnu*) }
 }
