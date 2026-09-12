# AGENTS.md — Working on YOS (`y/`)

Read this first when a task touches `y/`. It complements the repository-wide
[`AGENTS.md`](../AGENTS.md); anything not covered here (X toolchain builds,
libc rules, the X test matrix) is documented there.

## What YOS Is

YOS is a preemptive, ROM-based operating system for the 48K ZX Spectrum,
written entirely in hand-written Z80 assembly. The 16 KiB replacement ROM
stays compatible with esxDOS on divIDE, loads `shell.sys` from disk as an
XPRG process, and runs everything through a 50 Hz IM2 scheduler and named
service tables. Applications talk to it through the `yos_t` function table
(`include/yos.h`, ABI version 1) obtained with `query_service("yos")` over
`RST 0x18`.

## Documents To Read

| When you need | Read |
|---|---|
| the whole picture, memory map, source map | [docs/books/THE-BOOK-OF-YOS.md](docs/books/THE-BOOK-OF-YOS.md) |
| reset, ROM header, vector table, IM2, critical sections | [docs/books/the-book-of-yos/BOOT.md](docs/books/the-book-of-yos/BOOT.md) |
| lists, `sysobj_t`, ownership, `so_create` / `so_destroy` | [docs/books/the-book-of-yos/RESOURCE-ACCOUNTING.md](docs/books/the-book-of-yos/RESOURCE-ACCOUNTING.md) |
| heaps, block header, allocate / free / free-by-owner | [docs/books/the-book-of-yos/MEMORY-MANAGEMENT.md](docs/books/the-book-of-yos/MEMORY-MANAGEMENT.md) |
| thread object, states, startup stub, context switch, events | [docs/books/the-book-of-yos/THREADS.md](docs/books/the-book-of-yos/THREADS.md) |
| process object, no-parent model, `process_start`, `load_process`, `process_exit` | [docs/books/the-book-of-yos/PROCESSES.md](docs/books/the-book-of-yos/PROCESSES.md) |
| shared/private libraries, initializer ownership, reference cleanup | [docs/books/the-book-of-yos/LIBRARIES.md](docs/books/the-book-of-yos/LIBRARIES.md) |
| what the scheduler reclaims and when | [docs/books/the-book-of-yos/CLEANUP-RESOURCES.md](docs/books/the-book-of-yos/CLEANUP-RESOURCES.md) |
| services, RST 18, the `yos_t` and `gpx` tables | [docs/books/the-book-of-yos/SYSCALLS.md](docs/books/the-book-of-yos/SYSCALLS.md) |
| tick counters, timer chain, callback rules | [docs/books/the-book-of-yos/CLOCK.md](docs/books/the-book-of-yos/CLOCK.md) |
| XPRG descriptor, loader checks, building `shell.sys` | [docs/books/the-book-of-yos/PROGRAM-IMAGES.md](docs/books/the-book-of-yos/PROGRAM-IMAGES.md) |
| how to write kernel assembly | [docs/standards/YOS-ASSEMBLY-STYLE-GUIDE.md](docs/standards/YOS-ASSEMBLY-STYLE-GUIDE.md), on top of [x/docs/standards/Z80-CODING-STYLE.md](../x/docs/standards/Z80-CODING-STYLE.md) |
| release history | [CHANGELOG.md](CHANGELOG.md) |
| the public ABI | [include/yos.h](include/yos.h), [include/gpx.h](include/gpx.h), [include/dirent.h](include/dirent.h) |
| XPRG format reference | [x/src/xprog/README.md](../x/src/xprog/README.md) |
| host tools | [pkg/appmake/README.md](pkg/appmake/README.md), [pkg/microdrive/README.md](pkg/microdrive/README.md), [pkg/serial/README.md](pkg/serial/README.md) |
| microdrive driver harness | [tests/mdr-emu/README.md](tests/mdr-emu/README.md) |
| the old C-era kernel | `src/c/README.md` and `src/c/docs/`; [docs/books/the-book-of-yos/LEGACY-README-SNAPSHOT.md](docs/books/the-book-of-yos/LEGACY-README-SNAPSHOT.md) is its original README |

## Layout

```
y/
├── src/z80/        assembly kernel  -> bin/y/z80/spectrum/bin/yos-kernel.rom, shell.sys
│   ├── startup/    crt0rom.s (fixed ROM header), RAM init, vectors, critical sections
│   ├── kernel/     lists, heaps, threads, processes, events, timers, services, loader
│   ├── drivers/    clock, keyboard, Kempston mouse
│   ├── fs/         POSIX-style esxDOS filesystem and its RAM gates
│   ├── gpx/        libgpx v1.1.0-1-g0ef6f07 (GPL-2.0) + YOS integration
│   ├── main.s      kernel init sequence
│   └── linker.lk   ROM/RAM layout, reserved divIDE and Interface 1 addresses
├── src/c/          earlier C kernel -> yos.rom (kept buildable, not the focus)
├── include/        public headers for applications
├── pkg/            host tools: appmake, microdrive, serial -> bin/y/bin
├── tests/          kernel-z80, shell-yos, Fuse real-firmware runner, legacy apps/media
└── docs/           books/THE-BOOK-OF-YOS.md + books/the-book-of-yos/ (chapters), standards/ (style guide)
```

## Build

All builds use the staged X toolchain in `bin/x/bin` (`xas`, `xld`, `xar`,
`xcc`, `xprog`). If it is missing, `make -C y/src/z80` builds it first via
`make -C .. x`.

```bash
make -C y                      # both kernels (src/Makefile: targets c and z80)
make -C y/src/z80              # assembly kernel + shell.sys only
make -C y/src/c                # C-era kernel only
make -C y packages             # appmake, microdrive, serial -> bin/y/bin
make -C y clean
```

Outputs: `bin/y/z80/spectrum/bin/yos-kernel.rom` (must stay ≤ 16384 bytes —
the Makefile fails the link if `s__GSFINAL` passes `0x4000`), `shell.sys`,
and the link map under `build/yos-z80/yos-kernel.map`. Intermediate `.rel`
files and `libyos-kernel.lib` live in `build/yos-z80/`.

The kernel is linked from an archive so `xld` drops every routine the ROM does
not reference. `src/z80/Makefile` orders the archive deliberately (kernel,
then gpx, then the boot loader, with early filesystem/timer anchors) to pack code around the fixed divIDE holes;
keep that order when adding modules.

## Test

```bash
make -C y/src/z80 test         # kernel-z80: boot the ROM under libxz80
make -C y/tests/mdr-emu test   # microdrive driver harness (targets the C-era yos.rom)
make -C y/tests/mdr-emu stress # repeated microdrive smoke passes
```

`tests/kernel-z80/test_kernel.cpp` loads the exact production ROM and map.
The emulator defers `_boot_shell` until its RAM-gate fixture is populated;
no test-only ROM relink or ROM patch is used. It verifies: the fixed RST
and NMI bytes, the vector table and IM2 word, heap initialization, the public
`yos_t` wrappers, the `gpx` service, filesystem errno behaviour without a
firmware, XPRG CRC and relocation using the built `shell.sys`, process and
thread creation, three interrupt-driven context switches, event wakeup,
terminated-thread cleanup, and that the kernel never writes into ROM. Its
RAM-gate esxDOS fixture also runs the actual shell and self-registering
`shelllib.svc`, shared/private lifetime, initializer rollback and OOM cases.
Threading regressions force a second loader through a real IM2 context switch,
audit shared-state/framebuffer accesses, and check nested IFF preservation,
per-thread errors, timer-driven mouse state and independent GPX contexts.
Run it after any change to `src/z80/`.

For a repeatable visible Fuse cold boot, run
`python3 y/tests/fuse/run.py --esxdos build/yos-fuse/esxdos089` from the root;
see `tests/fuse/README.md`. It creates fresh media under `build/yos-fuse/`
from a user-supplied firmware distribution and does not modify Fuse settings.
Every esxDOS gate must mask IM2 until divIDE restores the scheduler's ROM.

Real-firmware validation of the replacement ROM is otherwise manual: boot
`yos-kernel.rom` as the base ROM in ZEsarUX (or Fuse) with a divIDE, an esxDOS
0.8.9 image and both `shell.sys` and `shelllib.svc` on the mounted disk, and
expect "Library OK" below the centred
greeting from `tests/shell-yos/shell.c`. The esxDOS harness in
`x/tests/tests/zx48/esxdos/` (`run_rom_firmware.py`) shows how to drive
ZEsarUX from a script if you need to automate it.

The `hello-yos` and `mdr*-yos` directories under `tests/` are application
builds that produce microdrive images for the C-era ROM. They target the historical C-era ABI
and still call `yos->printf`, which the current `include/yos.h` no longer
has, so they do not build against it and are not part of any default target.

## Rules Specific To `y/`

- **Build in `build/`, deliver to `bin/`, nothing anywhere else.** All
  intermediates go under `<repo>/build/` (`build/yos-z80/`, `build/yos/`,
  `build/hello-yos/`, `build/mdr-emu/`, …) and all outputs under
  `<repo>/bin/` (`bin/y/…`). Never create files inside `y/src/`, `y/tests/`,
  `y/docs/` or `y/pkg/` that are not source, and never write outside the
  repository. New Makefiles inherit `BUILD_DIR` and `DIST_DIR` from the root
  exactly as `src/z80/Makefile` does. This is the repository-wide rule from
  the root [`AGENTS.md`](../AGENTS.md); it is repeated here because `y/`
  harnesses have drifted before.
- **Assembly only.** No C in `src/z80/`; `shell.sys` is compiled from the
  C smoke fixture under `tests/shell-yos/` and is an application, not ROM.
- **One routine per module.** `name.s` defines `_name`; helpers and shared
  state go in `_name.s` / `_<subsystem>_state.s`. Never merge modules — the
  archive link relies on it.
- **`sdcccall(1)` everywhere.** First argument `HL`, second `DE`, rest on the
  stack, 16-bit result in `DE`. Document inputs, outputs, clobbers and whether
  a routine removes its own stack arguments in every routine header.
- **Fixed addresses are sacred.** The first 256 bytes of `crt0rom.s`, the
  `RESERVE` ranges in `linker.lk`, the IM2 word at `0x5EFF`, `_DATA` at
  `0x5B00` and the 16 KiB ROM limit are all checked by the build or the test.
- **The ROM links no libc and no X runtime.** Add a tiny `__helper` instead
  of importing one.
- **`yos_t` order is the ABI.** Append new entries at the end of both
  `include/yos.h` and the template in `kernel/_syscall_table_init.s`, bump
  `YOS_VERSION` in both public `yos.h` headers, `kernel/yos_version.s`, and
  `kernel/_image_load.s` together, and update
  `YOS_TABLE_SIZE` and `kernel/_yos_state.s`.
- **Vendored gpx is upstream code.** Fix bugs in `src/z80/gpx/`, do not
  restyle; record the upstream commit in `src/z80/gpx/README.md`.
- **Update the book.** A change to a kernel object layout, the boot sequence,
  the scheduler or the ABI must be reflected in the matching chapter under
  `docs/books/the-book-of-yos/` and in `CHANGELOG.md` (Unreleased section).
- **Markdown filenames** are `UPPER-CASE-WITH-HYPHENS.md`.

## Known Gaps

- No `thread_wait4events` / `thread_join`: the waiting state and the event
  wakeup scan exist, but nothing moves a thread onto the waiting list.
- Libraries support relocatable XPRG services with 1–255 exports. Fixed JP
  addresses, dependency chains, finalizers and explicit unloading are absent.
- `thread.hdr.owner` is normally zero and temporarily supplies the library
  allocation/registration owner during initialization. Never replace
  `thread.process` for this: it must keep the calling process alive.
- Keep process/library file validation, CRC and XL relocation shared through
  `_image_load.s`. Both process and service names are bounded.
- Processes have no parent field or wait/status relation. Do not confuse a
  system object's resource `owner` with process ancestry.
- Raw `so_create`, list routines and allocators require caller-held critical
  sections; `so_destroy` protects its unlink/free transaction. Public APIs
  protect shared state through publication, not only allocation. Preserve IFF
  and all flags in critical helpers; never use bare `EI` in a protected body.
- GPX contexts are independent process-owned allocations, not globals. Keep
  framebuffer read/modify/write sequences protected without locking entire
  compound drawings. Kernel errno/loader status are scheduler-virtualized;
  linked libc `errno` remains process-local and is a separate limitation.
- `tests/hello-yos` and `tests/mdr*-yos` target the removed `yos->printf`
  and the C-era microdrive driver; they need porting to ABI 1 and esxDOS.
