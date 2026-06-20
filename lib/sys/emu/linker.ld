/* Flat emulator memory map.
 *
 * The executable starts at 0x0000 and communicates with the host runner
 * through a fixed mailbox near the top of RAM:
 *
 *   0xff00..0xff02  process result/done flag
 *   0xff10..0xff3f  syscall request block
 *
 * The emu crt0 keeps the Z80 stack at 0xf000 so this mailbox remains stable.
 */

ENTRY(_entry)
OUTPUT_FORMAT(binary)
BINARY_RANGE(0x0000-0xFFFF)

SECTIONS {
  .text 0x0000 : { *(.text) }
}
