# `zx-esxdos-rom` platform

`zx-esxdos-rom` builds a complete 16 KiB replacement Spectrum ROM that boots
with divIDE and esxDOS 0.8.9 and executes its application directly from ROM.
It supplies the small base-ROM entry points needed by esxDOS and does not
require or bundle the Sinclair ROM. Matching esxDOS firmware and disk `SYS`
files are required; set `AutoBoot=0` in `SYS/CONFIG/ESXDOS.CFG`.

Code and constants remain in ROM. Startup at `0x0100` copies writable native
`_DATA` and ELF `.data` to RAM, including 48 bytes of fixed disk-call gates,
and initializes both BSS forms. Each gate pages in esxDOS with `RST 08`;
firmware pages itself out before the gate returns to the calling ROM code.
Pathnames and write input held in ROM use bounded temporary RAM copies while
firmware hides the base ROM. Read destinations remain writable RAM.

Both linker scripts reserve divIDE's `0x04C6`, `0x0562` and
`0x3D00`–`0x3DFF` instruction-fetch traps, with jump guards before each hole.
The CRT selects SP `0xFFFF` and IY `0x5C3A`, initializes the console and calls
`main` in ROM. Writable storage starts at `0x5B00`; the heap begins after it
and ends at the linker-defined `_STACK` boundary, `0xF000` by default.
This recovers 9,472 bytes from the former `0x8000` start. Stock esxDOS 0.8.9's
supported external file calls keep internal buffers in divIDE RAM and need
no permanent Spectrum workspace. Boot scratch is reclaimed after firmware
returns. The supplied disk example uses 100 bytes of static RAM,
including the gates, versus 5,330 bytes for the earlier `-Os` RAM-copy design;
stack and heap use are additional.

The default 4 KiB stack allowance is shared with the RAM disk target and is
not a disk-API minimum. A custom linker script can move `_STACK` together
with its matching reservation and, in GNU syntax, the RAM region end.
See the installed guide for an example and the temporary-input stack costs.

All implementation files and the target-private header live in this target.
There are no includes from a sibling target. The console, keyboard and font
retain the RAM target's behavior; the disk hooks also accept ROM pathnames
and write input. File descriptors, filesystem semantics and halt-on-exit
behavior are the same as that RAM target.

```sh
mkdir -p build/examples/zx-esxdos-rom
bin/x/bin/xcc -Os --platform=zx-esxdos-rom --oformat=binary \
  x/examples/zx-esxdos-rom/diskio.c \
  -o build/examples/zx-esxdos-rom/DISKIO.ROM
```

Install the resulting image as the machine's 16 KiB base ROM, keeping the
separate divIDE esxDOS firmware and matching disk files. It starts on reset;
no BASIC loader is needed. BASIC, esxDOS dot commands, NMI-browser operation
and 128K banking are outside this target's contract.

See the [installed ROM guide](../../docs/dist/man/ZX-ESXDOS-ROM.md) and
[ROM disk example](../../examples/zx-esxdos-rom/README.md).

The preceding direct-ROM layout passed 43 configurations covering native
S/M/L and GNU M, both caller ABIs, filesystem and ROM-input probes, examples
and a 10 KiB constant/initialization fixture. All 43 recorded-tool images
match the successful firmware executions byte for byte. Twelve deterministic
ROM ABI lanes also passed, each with 553 checks and 891 hostile firmware
calls, as did all four original Spectrum MCP modes with the tools recorded
for that layout. All 18,047 compiler regression variants passed with their
recorded tools; that linker separately passed 103 normal/sanitized component
tests. The
[direct ROM execution record](../../tests/tests/zx48/esxdos/xip-validation-2026-09.json)
retains the validation and tool identities for that layout. The
[reclaimed-RAM record](../../tests/tests/zx48/esxdos/compact-ram-validation-2026-09.json)
records 67 successful stock-firmware cold boots with FAT16/FAT32 and 18
deterministic ABI lanes using the final tools. Four RAM low-arena example
images match successful BASIC-loader boots. The final linker passes 156
tests in normal and sanitized builds, plus executable reserved-hole and
REL/ELF addend checks. All 48 latest XCC benchmark cells pass with no size
or cycle changes.
All four original Spectrum MCP modes pass with the final staged tools.

The earlier [validation record](../../tests/tests/zx48/esxdos/rom-validation-2026-09.json)
is retained for the superseded whole-application RAM-copy implementation.
See the [reproduction commands](../../tests/tests/zx48/esxdos/README.md).
