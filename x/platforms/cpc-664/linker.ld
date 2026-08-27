/* Firmware-hosted Amstrad CPC 664 RAM program. */
ENTRY(_entry)
OUTPUT_FORMAT(binary)
RESERVE(0x9F00-0xFFFF)

MEMORY {
  ram (rwx) : ORIGIN = 0x4000, LENGTH = 0x5F00
}

SECTIONS {
  .text 0x4000 : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(_CODE _CONST _INITIALIZER _GSINIT _GSFINAL)
  } >ram
  .data : { *(.data .data.*) *(_DATA _INITIALIZED) } >ram
  .bss : { *(.bss .bss.*) *(_BSS _HEAP) } >ram
}
