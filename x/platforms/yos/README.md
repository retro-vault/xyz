# YOS XCC backend

`--platform=yos` links a relocatable XL process against the ordinary X libc.
The CRT keeps the stack supplied by YOS, initializes C storage, resolves the
`"yos"` service through `RST 0x18`, and calls `main`.

All hosted facilities cross the `yos_t` table: memory uses
`allocate_memory`/`free_memory`, and POSIX file and directory calls use the
esxDOS-backed entries. Standard console output is intentionally silent until
the application installs a `yos_putchar_hook_t` with
`yos_set_putchar_hook()`; Alto can later provide that hook from its console
window. Raw keyboard transitions remain available as `yos->read_key()`.

Build and package a process with:

```sh
bin/x/bin/xcc -Os --platform=yos app.c -o build/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 1 \
  build/app.xl -o bin/y/z80/spectrum/bin/app.sys
```

The linker output must remain XL. Do not select a fixed-address or binary
output format for a YOS process.

The staged header describes the complete 98-byte ABI 1 table, including
process loading, private/shared XPRG libraries and the appended
`shrink_memory` entry. `load_library` returns a
relocated direct-call interface retained until the calling process exits.
Kernel shared-state syscalls use IFF-preserving critical sections. Raw kernel
errno/loader status follow the running thread, but linked libc `errno` remains
process-local. GPX contexts are independent process-owned allocations; the
physical framebuffer is shared. See the
[YOS concurrency contract](../../../y/docs/books/programming-yos/MEMORY-TIME-AND-CONCURRENCY.md).
