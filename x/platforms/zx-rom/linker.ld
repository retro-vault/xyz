/* ZX Spectrum ROM image layout.
 *
 * ROM occupies 0x0000-0x3FFF. Writable runtime state lives in RAM.
 * CLI options still override anything set here.
 *
 * Important current limitation:
 * xcc still emits ordinary writable globals directly into .data instead of a
 * full ROM-source / RAM-destination split. So this script describes the right
 * model, but fully correct initialized-data support still depends on future
 * compiler-side section splitting.
 */

ENTRY(_entry)
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
    *(.initialized .initialized.*)
  } >ram AT>rom
  _data_load = LOADADDR(.data);

  .bss : {
    *(.bss .bss.*)
    *(COMMON)
  } >ram

  /DISCARD/ : { *(.comment) *(.note*) }
}
