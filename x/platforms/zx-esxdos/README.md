# `zx-esxdos` platform

`zx-esxdos` builds a ZX Spectrum 48K RAM program that uses resident esxDOS
firmware on divIDE or compatible hardware. It loads at `0x8000`, keeping
clear of the active BASIC loader. It requires a normally booted Sinclair
ROM and esxDOS. After application entry, its standalone runtime can reuse
`0x5B00`–`0x7FFF`: supported stock 0.8.9 file calls keep internal buffers in
divIDE RAM and need no permanent Spectrum workspace there.

The assembly CRT disables interrupts, selects SP `0xFFFF` and IY `0x5C3A`,
clears BSS, copies initialized storage, initializes the bitmap console, and
calls `main`. The heap extends from the linked image end to the linker
`_STACK` boundary, `0xF000` by default. The shared 4 KiB stack allowance is
configurable with a copied linker script, not a disk-API minimum. Program exit records the result,
attempts to close disk descriptors 3–18, and halts.

The [low-RAM example](../../examples/zx-esxdos/lowram.c) initializes a
private arena below `0x8000` after entry and uses it for a 9,216-byte disk
round trip. The default heap remains above the linked image. Reclaiming
low RAM rules out returning to BASIC or using its ROM/interrupt services.

This directory contains its own console, keyboard scanner, Tamsyn font and
platform hooks. The console, keyboard, and font sources match `zx-ram`
byte-for-byte; no target imports a sibling's sources. Descriptors 0–2 retain
keyboard/screen behavior; disk descriptors are provided through esxDOS.

Build the included disk example:

```sh
mkdir -p build/examples/zx-esxdos
bin/x/bin/xcc -Os --platform=zx-esxdos --oformat=binary \
  x/examples/zx-esxdos/diskio.c -o build/examples/zx-esxdos/DISKIO.BIN
```

Copy `DISKIO.BIN` to the esxDOS disk, then enter:

```text
CLEAR 32767
LOAD *"DISKIO.BIN" CODE 32768
RANDOMIZE USR 32768
```

See the [installed esxDOS guide](../../docs/dist/man/ZX-ESXDOS.md) and the
[example](../../examples/zx-esxdos/README.md) for the API and disk workflow.
