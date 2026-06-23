ENTRY(_start)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0x3FFF)

MEMORY {
  rom (rx)  : ORIGIN = 0x0000, LENGTH = 0x4000
  ram (rwx) : ORIGIN = 0x8000, LENGTH = 0x8000
}

SECTIONS {
  .text : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(.vectors)
    . = ALIGN(2);
  } >rom

  _rom_end = .;
  ASSERT(_rom_end <= 0x4000, "ROM overflow!");

  .data : {
    *(.data .data.*)
  } >ram AT>rom
  _data_load = LOADADDR(.data);

  .bss : {
    *(.bss .bss.*)
    *(COMMON)
  } >ram

  /DISCARD/ : { *(.comment) *(.note*) }
}
