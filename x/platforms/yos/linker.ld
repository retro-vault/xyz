/* Relocatable YOS application image. */
ENTRY(_entry)
OUTPUT_FORMAT(xl)

SECTIONS {
  .text 0 : { *(.text) }
}
