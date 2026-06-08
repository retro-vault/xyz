/* CP/M COM-style memory map.
 *
 * Default placement:
 *   .text at 0x0100
 *   .data / .bss follow naturally in the transient program area
 *
 * CLI options override this script.
 */

ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0100-0xFFFF)

SECTIONS {
  .text 0x0100 : { *(.text) }
}
