/* Execute from ROM; only writable data and RST gates occupy RAM. */
ENTRY(__zx_rom_reset)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0x3FFF)
RESERVE(0x04C6-0x04C6)
RESERVE(0x0562-0x0562)
RESERVE(0x3D00-0x3DFF)
RESERVE(0xF000-0xFFFF)

MEMORY {
  rom (rx)  : ORIGIN = 0x0000, LENGTH = 0x4000
  ram (rwx) : ORIGIN = 0x5B00, LENGTH = 0x9500
}

/* Resident sections follow the explicit header address in order. */
SECTIONS {
  _HEADER 0x0000 : { *(_HEADER) }
  _CODE : { *(_CODE) }
  .text : { *(.text) }
  _CONST : { *(_CONST) }
  .rodata : { *(.rodata) }
  _INITIALIZER : { *(_INITIALIZER) }
  _GSINIT : { *(_GSINIT) }
  _GSFINAL : { *(_GSFINAL) }
  _DATA : { *(_DATA) } >ram AT>rom
  .data : { *(.data) } >ram AT>rom
  _INITIALIZED : { *(_INITIALIZED) } >ram
  _BSS : { *(_BSS) } >ram
  .bss : { *(.bss) } >ram
  _HEAP : { *(_HEAP) } >ram
  _STACK 0xF000 : { *(_STACK) }
  /DISCARD/ : { *(.comment) *(.note*) }
}
