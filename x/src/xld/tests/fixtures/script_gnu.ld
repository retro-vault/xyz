ENTRY(_gnu_entry)
OUTPUT_FORMAT(binary)
RESERVE(0x0100-0x010F)
BINARY_RANGE(0x0000-0x3FFF)

MEMORY {
  ROM (rx) : ORIGIN = 0x0200, LENGTH = 0x2000
  RAM (rw) : ORIGIN = 0x4000, LENGTH = 0x1000
}

SECTIONS {
  .text : { *(.text) } > ROM
  .data 0x4100 : { *(.data) }
  .bss : { *(.bss) } > RAM
}
