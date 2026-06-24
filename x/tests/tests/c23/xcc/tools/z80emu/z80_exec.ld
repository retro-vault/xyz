ENTRY(__xc_test_start)

SECTIONS
{
  . = 0x0000;

  .text : {
    *(.text)
    *(.rodata)
    *(.rodata*)
  }

  .data : {
    *(.data)
  }

  .tdata : {
    *(.tdata)
    *(.tdata*)
  }

  .bss (NOLOAD) : {
    *(.bss)
    *(.bss*)
    *(COMMON)
  }
}
