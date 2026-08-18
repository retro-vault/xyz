/* ZX Spectrum 48K replacement ROM. */
ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0x3FFF)
RESERVE(0xF000-0xFFFF)

MEMORY {
  rom (rx)  : ORIGIN = 0x0000, LENGTH = 0x4000
  ram (rwx) : ORIGIN = 0x5B00, LENGTH = 0x9500
}

SECTIONS {
  .vectors 0x0000 : { *(.vectors) *(_HEADER) } >rom
  .text : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(_CODE _CONST _INITIALIZER _GSINIT _GSFINAL)
  } >rom
  .data : {
    *(.data .data.*)
    *(_DATA)
  } >ram AT>rom
  .initialized : { *(_INITIALIZED) } >ram
  .bss : { *(.bss .bss.*) *(_BSS _HEAP) } >ram
  /DISCARD/ : { *(.comment) *(.note*) }
}
