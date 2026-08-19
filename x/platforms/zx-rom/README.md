# `zx-rom` platform

`zx-rom` builds a complete 16 KiB ZX Spectrum 48K replacement ROM covering
`0x0000`–`0x3FFF`. It owns the reset/RST/interrupt/NMI vectors, leaves the ULA
display at `0x4000`–`0x5AFF`, begins writable C state at `0x5B00`, and reserves
`0xF000`–`0xFFFF` for the descending stack.

Initialized `_DATA` runs from RAM but has a packed load image in ROM. The GNU
linker script describes this with `AT>rom`; the SDCC-style script uses
`COPY _DATA`. Startup copies `s__DATA_LOAD`/`l__DATA_LOAD`, processes the normal
initializer area, clears BSS, initializes the console, and calls `main`.

Every backend routine is hand-written Z80 assembly. This directory carries its
own YOS-derived bitmap renderer, proportional Tamsyn stream, and keyboard
scanner; it has no Sinclair-service or non-target platform dependency.
The public `<stdio.h>` header exposes non-blocking `trygetchar()`; blocking
`getchar` and standard input are loops around the same assembly poller.

Build and run in Fuse:

```sh
bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary main.c -o app.rom
fuse --machine 48 --rom-48 app.rom
```

See the [complete ZX Spectrum 48K guide](../../docs/howtos/ZX-SPECTRUM-48K.md).
