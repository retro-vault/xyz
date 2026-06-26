/* Flat emulator memory map.
 *
 * The executable starts at 0x0000 and communicates with the host runner
 * through a fixed mailbox near the top of RAM:
 *
 *   0xff00..0xff02  process result/done flag
 *   0xff10..0xff3f  syscall request block
 *
 * The emulator crt0 currently starts the Z80 stack at 0xfa00. The default
 * heap is not preallocated in the image; it begins at the end of the linked
 * program and may grow upward until the fixed emu heap ceiling at 0xf000.
 */

ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0xFFFF)

SECTIONS {
  .text 0x0000 : { *(.text) }
}
