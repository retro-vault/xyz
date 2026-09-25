# YOS XCC backend

`--platform=yos` links a relocatable XL process against the ordinary X libc.
The CRT keeps the stack supplied by YOS, initializes C storage, resolves the
`"yos"` service through `RST 0x18`, and calls `main`.

All hosted facilities cross the `yos_t` table. Standard allocation uses a
current-bank-only request so its 16-bit pointers remain valid; raw
`allocate_memory`/`free_memory` calls search all banks and use
`yos_user_ptr_t`. POSIX file and directory calls use the
esxDOS-backed entries. Standard console output is intentionally silent. A
process that needs a console must query its console service explicitly. Raw
keyboard transitions remain available as `yos->read_key()`.

Build and package a process with:

```sh
bin/x/bin/xcc -Os --platform=yos app.c -o build/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 6 \
  build/app.xl -o bin/y/arch/48/app.prc
```

The linker output must remain XL. Do not select a fixed-address or binary
output format for a YOS process.

The staged `yos.h` describes the complete grouped 154-byte ABI 6 table and
its filesystem data structures;
`yos.inc` publishes every byte offset for assembly callers. The table includes
the detected ROM model, banked allocation, process loading, private/shared
XPRG libraries,
`shrink_memory`, `wait_event`, command execution, print hooks, and the complete graphics API. Event waits
block the caller in the scheduler until signalled. `load_library` returns a
packed far-function table retained until the calling process exits.
Disk enumeration is available only as `yos_t.enumerate_disks`; it has no
duplicate global wrapper.
Kernel shared-state syscalls use IFF-preserving critical sections. Raw kernel
errno/loader status follow the running thread, but linked libc `errno` remains
process-local. GPX contexts are independent process-owned allocations; the
physical framebuffer is shared. See the
[YOS concurrency contract](../../../y/docs/books/programming-yos/MEMORY-TIME-AND-CONCURRENCY.md).
