/* ZX Spectrum RAM-loaded memory map.
 *
 * Default placement:
 *   .text at 0x8000
 *   .data / .bss follow naturally in upper RAM
 *
 * CLI options override this script.
 */

ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x8000-0xFFFF)

SECTIONS {
  .text 0x8000 : { *(.text) }
}
