/* Generic flat Z80 memory map.
 *
 * Default placement:
 *   .text at 0x0000
 *   .data / .bss follow naturally after earlier sections
 *
 * CLI options override this script.
 */

ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0xFFFF)

SECTIONS {
  .text 0x0000 : { *(.text) }
}
