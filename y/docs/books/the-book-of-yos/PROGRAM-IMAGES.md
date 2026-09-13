# Program and Service Images (`XPRG`)

YOS uses the X tools `XPRG` version 1 container for disk-resident programs and
libraries. A program is an XPRG **process** image. A loadable library is an
XPRG **service** image. Both contain an ordinary relocatable XL payload.

`xprog` is the canonical packager and format validator. Its complete format
reference is in [`x/src/xprog/README.md`](../../../../x/src/xprog/README.md).

## Descriptor

Every image starts with a 64-byte little-endian descriptor:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | `XPRG` magic |
| 4 | 1 | format version, currently 1 |
| 5 | 1 | kind: process (1) or service (2) |
| 6 | 1 | image ABI version |
| 7 | 1 | fixed-load, entry-present and jump-table flags |
| 8 | 2 | metadata size |
| 10 | 2 | XL payload offset |
| 12 | 4 | XL payload size |
| 16 | 4 | IEEE CRC-32 of the complete XL payload |
| 20 | 4 | stable image ID |
| 24 | 2 | preferred load address |
| 26 | 2 | XL code-relative entry point |
| 28 | 2 | usable process stack size |
| 30 | 2 | minimum YOS ABI version |
| 32 | 2 | service jump-table offset |
| 34 | 2 | service jump-table entry count |
| 36 | 4 | reserved, zero |
| 40 | 16 | NUL-padded image name |
| 56 | 8 | reserved, zero |

Process metadata ends at byte 64 and is followed immediately by the XL file.
Service metadata can also contain an ordered table of three-byte `JP nn`
entries before the XL payload.

## Process loading

`yos_t::load_process(path)` (`kernel/process_load.s`, using the shared
`kernel/_image_load.s` core) opens an XPRG file
through the ROM's POSIX/esxDOS layer and accepts process images. It validates the descriptor, required YOS
version, payload CRC, XL header, relocation table, code bounds, entry point,
fixed-load requirement and stack size. The XL payload is version 2: a 12-byte
header, the code, and then the relocation table. The whole payload is read
into one heap block, so the JP metadata and XL header precede the code and the
consumed relocation records trail it. The relocator patches the code at its
existing address, without allocating a second code copy. After metadata has
been consumed, `__image_retain` first shrinks the owned heap block to the
relocated code end through the public `shrink_memory` routine, freeing the
trailing relocation table, then splits the block immediately before the
resident code (or compact service table). The leading metadata block is freed
by the loader's ordinary final cleanup. The code does not move. Only the
code (plus the compact service table) stays allocated; the relocation table,
which can be a quarter of the image, is returned to the heap, and because it
was the top of the block its release coalesces with the free space above.
The loader creates the process and its main thread and transfers ownership of
only the retained block. The descriptor remains on the loader stack.

The descriptor's stack size is usable application stack. YOS adds its private
22-byte scheduler context when it creates the main thread. XPRG names can hold
15 characters; the current kernel process object retains the first seven.

`yos_t::process_load_error` points at a byte containing one of the
`YOS_PROCESS_LOAD_*` values in `yos.h`. The synchronous loading call writes
the current thread's value; the scheduler saves and restores it across context
switches. A competing or recursive load returns immediately with `BUSY`. A
service image passed to `load_process` is rejected with
`YOS_PROCESS_LOAD_NOT_PROCESS`. ABI 1 includes `load_library(path, flags)` for
service images; see [Libraries](LIBRARIES.md) for relocation,
self-registration, sharing and automatic release.

At boot the ROM opens `shell.sys` on the current esxDOS drive and directory
(`kernel/boot_shell.s`), loads it as a process, and only then arms the
scheduler. The build creates the current smoke-test `shell.sys` from
`y/tests/shell-yos/shell.c`: it loads `shelllib.svc`, calls the relocated
library interface, queries `gpx`, centres a greeting and `Library OK`, and
loops forever. The production image gets both `_entry` and the
`RST 0x18; RET` `query_service` stub from the XCC `--platform=yos` backend.

Every load error leaves one of the `YOS_PROCESS_LOAD_*` codes in the byte
`process_load_error` points at:

| Code | Meaning |
|---:|---|
| 0 | `OK` |
| 1 | `NOT_FOUND` — `open` failed |
| 2 | `NO_MEMORY` — image does not fit in `__heap` |
| 3 | `READ_ERROR` — short read |
| 4 | `INVALID_IMAGE` — bad magic, version, flags, XL header, relocation or bounds |
| 5 | `START_ERROR` — `process_start` failed (`__sys_heap` or stack allocation) |
| 6 | `NOT_PROCESS` — the image is an XPRG service |
| 7 | `REQUIRES_NEWER_OS` — descriptor's minimum ABI exceeds `YOS_VERSION` |
| 8 | `BAD_CHECKSUM` — payload CRC-32 mismatch |
| 9 | `BUSY` — a competing or recursive load |
| 10 | `NO_PROCESS` — library load without a client process |
| 11 | `INIT_ERROR` — library initializer returned failure |

## Building a process

Compile and link as a relocatable XL with the YOS backend, then package it
with a nonzero stack requirement and the oldest compatible YOS ABI:

```sh
mkdir -p build/examples/yos bin/y/examples
bin/x/bin/xcc -Os --platform=yos app.c -o build/examples/yos/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 1 \
  build/examples/yos/app.xl -o bin/y/examples/app.sys
```

Both images declare ABI 1, the clean baseline for the complete current table.
The shell also checks `yos->version() >= YOS_VERSION` before accessing
`load_library`.

`y/src/z80/Makefile` (`$(SHELL_XL)` and `$(SHELL_OUTPUT)`) is the reference
recipe.

Use `xprog --service` for a library intended to be registered as a named YOS
service; it must describe its exported jump-table entries. See the practical
[library guide](../programming-yos/LOADABLE-LIBRARIES.md).
