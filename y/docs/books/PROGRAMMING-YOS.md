# Programming YOS

This book teaches application programming for YOS with the X toolchain. It
starts with one tiny C process, then adds services, the C library, files,
concurrency, input, and graphics. The final two chapters are complete call
references, each entry backed by a working example.

## Chapters

Read top to bottom the first time; each chapter builds on the one before it.

| # | Chapter | Covers |
|---|---|---|
| 1 | [Your First Process](programming-yos/YOUR-FIRST-PROCESS.md) | building, packaging, and running one process |
| 2 | [Services and Console](programming-yos/SERVICES-AND-CONSOLE.md) | `query_service`, the `yos_t` table, console output |
| 3 | [Memory, Time, and Concurrency](programming-yos/MEMORY-TIME-AND-CONCURRENCY.md) | `malloc`/far allocation, `clock_ticks`, threads, critical sections, the shared-state contract |
| 4 | [Files, Input, and Graphics](programming-yos/FILES-INPUT-AND-GRAPHICS.md) | the esxDOS filesystem, keyboard, mouse, and an introduction to GPX |
| 5 | [Loadable Libraries](programming-yos/LOADABLE-LIBRARIES.md) | packaging and using shared/private XPRG libraries |
| 6 | [YOS API Reference](programming-yos/YOS-API-REFERENCE.md) | every non-graphics `yos_t` call, with an example each |
| 7 | [GPX API Reference](programming-yos/GPX-API-REFERENCE.md) | every graphics call, with an example each, plus a complete worked program |

YOS applications are ordinary C programs linked as relocatable **XL** files.
`xprog` wraps an XL file in the XPRG process container that YOS loads from an
esxDOS filesystem. The XCC platform name is `yos`:

```sh
mkdir -p build/examples/yos bin/y/arch/48
bin/x/bin/xcc -Os --platform=yos app.c -o build/examples/yos/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 1 \
  build/examples/yos/app.xl -o bin/y/arch/48/app.prc
```

Keep the XL file — it is the linker's relocatable output and is useful for
inspection. The `.prc` file is the installable YOS process; do not ask XCC
for a flat binary when targeting `--platform=yos`. For the disk image,
launcher scripts, and how to actually boot the result, see
[Running your program](programming-yos/YOUR-FIRST-PROCESS.md#running-your-program).

## What the backend supplies

The YOS CRT clears BSS, copies initialized C data, asks RST `0x18` for the
`"yos"` service, caches its ABI table, calls `main`, and turns a return from
`main` into `exit_process`. It runs on the stack the process loader
allocated for it — there is no fixed application address and no private
static heap.

The ordinary C library remains available, but its machine-facing parts are
replaced by the YOS platform archive:

- `malloc`, `free`, and `realloc` draw from YOS-owned memory in the
  currently executing bank. Raw `yos_user_ptr_t` allocation, by contrast,
  scans every configured bank.
- POSIX file and directory calls go through the YOS esxDOS service table.
- `putchar`, `puts`, `printf`, and writes to file descriptors 1 and 2 are
  deliberately silent; applications that want console output query an
  explicit console service instead.
- `getchar` reports `EOF` and `trygetchar` reports no character available.
  Use the raw keyboard service until Alto — the planned windowing layer —
  supplies a console window.
- Unix wall-clock calls fail with `ENOSYS`. Use `clock_ticks()` for
  monotonic 50 Hz time instead.

This design keeps libc portable: target code either computes locally or
crosses the published YOS ABI. It never calls Spectrum ROM routines or
esxDOS directly.

For kernel internals — boot, object layouts, scheduling, cleanup, and XPRG
loading — read [The Book of YOS](THE-BOOK-OF-YOS.md). The runnable companion
sample is [`x/examples/yos/hello.c`](../../../x/examples/yos/hello.c).

## Important limits

The current public ABI is version 6. It has no blocking console input, no
Unix wall clock, no thread join, no process wait/status channel, and no
explicit library unload. A process keeps no stored parent relationship, even
when another process created or loaded it, so `exit(int)` cannot report its
numeric status back to a creator. ABI 1 can load relocatable XPRG service
images as private or reference-counted shared libraries, but dependency
chains and finalizers are not implemented.

Kernel shared-state calls are serialized, and the raw filesystem and loader
error cells follow the running thread. Linked libc `errno` remains
process-local rather than thread-local; GPX contexts are independently
allocated and process-owned, but the screen itself is shared; and
caller-owned buffers and service state still need coordination between
callers. The precise contract is spelled out in
[Memory, Time, and Concurrency](programming-yos/MEMORY-TIME-AND-CONCURRENCY.md).

Paths and directory entries reflect the underlying esxDOS 8.3 filesystem.
YOS supplies 48K, Spectrum 128K, and Spectrum Next bank mappers while
keeping the same divIDE/esxDOS filesystem contract across all three. Code
that stays on libc plus `yos.h` remains independent of those hardware
details.
