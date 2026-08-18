/* ZX Spectrum 48K RAM program. */
ENTRY(_entry)
OUTPUT_FORMAT(binary)
RESERVE(0xF000-0xFFFF)

MEMORY {
  ram (rwx) : ORIGIN = 0x5CCB, LENGTH = 0x9335
}

SECTIONS {
  .text 0x5CCB : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(_CODE _CONST _INITIALIZER _GSINIT _GSFINAL)
  } >ram
  .data : { *(.data .data.*) *(_DATA _INITIALIZED) } >ram
  .bss : { *(.bss .bss.*) *(_BSS _HEAP) } >ram
}
