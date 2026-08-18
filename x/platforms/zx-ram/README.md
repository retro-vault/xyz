# `zx-ram` platform

`zx-ram` is the minimal ZX Spectrum 48K RAM backend. It links a flat binary at
`0x5CCB`, immediately after the Sinclair ROM system variables, and reserves
`0xF000`–`0xFFFF` for the descending stack.

`crt0.s` disables interrupts, sets SP to `0xFFFF`, clears BSS, copies ordinary
initialized storage, initializes the shared bitmap console, calls `main`, and
passes its result to `_exit`. `heap_region.s` returns the range from the linked
image end to `0xF000`.

Every platform implementation file is Z80 assembly. This directory contains
its own `console.s`, `keyboard.s`, and `tamsyn.s`; it has no dependency on a
non-target platform directory. `<conio.h>` exposes the assembly `kbhit()`
primitive, which returns the current ASCII key or zero without waiting.
Blocking `getchar`/standard input are derived from that poller.
Descriptors 1–2 provide Tamsyn screen output; files and the wall clock are
deliberately unsupported.

Build and package:

```sh
bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary main.c -o app.bin
bin/x/bin/xprog --tap app.bin -o app.tap --name APP
bin/x/bin/xprog --tzx app.bin -o app.tzx --name APP
```

See the [complete ZX Spectrum 48K guide](../../docs/howtos/ZX-SPECTRUM-48K.md).
