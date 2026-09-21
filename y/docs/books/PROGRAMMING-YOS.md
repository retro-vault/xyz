# Programming YOS

This book teaches application programming for YOS with the X toolchain. It
starts with one tiny C process, then adds services, libc, files, concurrency,
input, and graphics. The last two chapters are complete call references with
a small example for every public operation.

YOS applications are ordinary C programs linked as relocatable **XL** files.
`xprog` wraps an XL file in the XPRG process container that YOS loads from an
esxDOS filesystem. The XCC platform name is `yos`:

```sh
mkdir -p build/examples/yos bin/y/z80/spectrum/bin
bin/x/bin/xcc -Os --platform=yos app.c -o build/examples/yos/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 1 \
  build/examples/yos/app.xl -o bin/y/z80/spectrum/bin/app.prc
```

Keep the XL file: it is the linker's relocatable result and is useful for
inspection. The `.prc` file is the installable YOS process. Do not ask XCC for
a flat binary when using `--platform=yos`.

## What the backend supplies

The YOS CRT clears BSS, copies initialized C data, asks RST `0x18` for the
`"yos"` service, caches its ABI table, calls `main`, and turns a return from
`main` into `exit_process`. It uses the stack allocated by the process loader;
there is no fixed application address or private static heap.

The normal C library remains available. Its machine-facing parts are replaced
by the YOS platform archive:

- `malloc`, `free`, and `realloc` use YOS-owned process memory.
- POSIX file and directory calls use the YOS esxDOS service table.
- `putchar`, `puts`, `printf`, and writes to file descriptors 1 and 2 are
  deliberately silent until a process installs a character hook.
- `getchar` reports `EOF` and `trygetchar` reports no character. Use the raw
  keyboard service until Alto supplies a console window.
- Unix wall-clock calls fail with `ENOSYS`; use `clock_ticks()` for monotonic
  50 Hz time.

This keeps libc portable: target code either computes locally or crosses the
published YOS ABI. It does not call Spectrum ROM routines or esxDOS directly.

For kernel internals—boot, object layouts, scheduling, cleanup, and XPRG
loading—read [The Book of YOS](THE-BOOK-OF-YOS.md). The runnable companion is
[`x/examples/yos/hello.c`](../../../x/examples/yos/hello.c).

## Important limits

The current public ABI is version 2. It has no blocking console input, Unix
wall clock, thread join, event wait call, process wait/status channel, or
explicit library unload. A process has no stored parent relationship, even
when another process created or loaded it, and `exit(int)` cannot report its
numeric status to the creator. ABI 1 can load relocatable XPRG service images
as private or reference-counted shared libraries; dependencies and finalizers
are not implemented.

Kernel shared-state calls are serialized and the raw filesystem and loader
error cells follow the running thread. Linked libc `errno` remains
process-local, GPX contexts are independently allocated and process-owned but
the screen is shared, and caller-owned buffers and service state still need
coordination. The precise contract is in
[Memory, Time, and Concurrency](programming-yos/MEMORY-TIME-AND-CONCURRENCY.md).

Paths and directory entries reflect the underlying esxDOS 8.3 filesystem.
YOS currently targets the 48K Spectrum with divIDE/esxDOS. Code that stays on
libc plus `yos.h` remains independent of those firmware details.
