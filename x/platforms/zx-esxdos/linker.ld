/* ZX Spectrum 48K RAM program with resident esxDOS firmware. */
ENTRY(_entry)
OUTPUT_FORMAT(binary)
RESERVE(0xF000-0xFFFF)

MEMORY {
  ram (rwx) : ORIGIN = 0x8000, LENGTH = 0x7000
}

SECTIONS {
  .text 0x8000 : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(_CODE _CONST _INITIALIZER _GSINIT _GSFINAL)
  } >ram
  .data : { *(.data .data.*) *(_DATA _INITIALIZED) } >ram
  .bss : { *(.bss .bss.*) *(_BSS _HEAP) } >ram
  _STACK 0xF000 : { *(_STACK) }
}
