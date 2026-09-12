![status.badge] [![language.badge]][language.url] [![standard.badge]][standard.url]

# YOS

`yos` is a preemptive, ROM-based operating system for the 48K ZX Spectrum,
written entirely in hand-written Z80 assembly. It boots from a 16 KiB
replacement ROM that stays compatible with esxDOS on a divIDE interface,
loads its shell from disk as a relocatable XPRG process, supports reference-counted
shared/private XPRG libraries, and then runs
everything through a 50 Hz interrupt-driven round-robin scheduler and named
service tables.

## Layout

| Path | Contents |
|---|---|
| `src/z80/` | the assembly kernel: `startup/`, `kernel/`, `drivers/`, `fs/` (esxDOS), `gpx/` (vendored libgpx), `main.s`, `linker.lk`; builds `yos-kernel.rom` and `shell.sys` |
| `src/c/` | the earlier C-and-assembly kernel, still buildable as `yos.rom`, with its own copy of the old chapter docs |
| `include/` | public headers used by YOS applications: `yos.h` (kernel ABI 9), `gpx.h`, `dirent.h`, `microdrive/microdrive.h` |
| `pkg/` | host tools staged into `bin/y/bin/`: [`appmake`](pkg/appmake/README.md), [`microdrive`](pkg/microdrive/README.md), [`serial`](pkg/serial/README.md) |
| `tests/` | `kernel-z80/` emulated kernel test, `shell-yos/` boot shell fixture, `hello-yos/` and `mdr*-yos/` apps, [`mdr-emu/`](tests/mdr-emu/README.md) microdrive harness, `microdrives/` and `tapes/` media |
| `docs/books/` | [Programming YOS](docs/books/PROGRAMMING-YOS.md) for application authors and [The Book of YOS](docs/books/THE-BOOK-OF-YOS.md) for kernel internals |
| `docs/standards/` | [YOS assembly style guide](docs/standards/YOS-ASSEMBLY-STYLE-GUIDE.md) |

## Build and Test

```bash
make -C y                  # both kernels; needs the staged X toolchain in bin/x
make -C y/src/z80          # assembly kernel only -> bin/y/z80/spectrum/bin/yos-kernel.rom + shell.sys
make -C y/src/z80 test     # boot the ROM under libxz80 and exercise the kernel
make -C y packages         # host tools -> bin/y/bin
```

The assembly kernel is built with `xas`/`xld`/`xar` from `bin/x/bin`;
The build also emits `shelllib.svc`; copy it alongside `shell.sys` on the
esxDOS drive. The shell calls its relocated, self-registered interface and
shows "Library OK". `shell.sys` is compiled as a relocatable XL application by the XCC `yos`
backend and packaged with `xprog`. Details, including how to validate the ROM against real esxDOS, are
in [AGENTS.md](AGENTS.md).

To show the shell in Fuse with real esxDOS firmware:

```sh
python3 y/tests/fuse/run.py --esxdos build/yos-fuse/esxdos089
```

Supply an extracted esxDOS distribution at that path. See the
[Fuse runner guide](tests/fuse/README.md) for dependencies and cold-boot details.

## System Overview

1. **Boot (`src/z80/startup/crt0rom.s`)** — the CPU starts at `0x0000`; the
   first 256 bytes are an esxDOS-compatible header (RST 08 and NMI belong to
   the firmware, RST 10 is an immediate `RET`, RST 18-30 jump through a
   writable RAM table). `__startup_init` zeroes BSS, copies the vector table
   and initialized data from ROM to RAM and builds the 96-byte service table.
2. **Kernel bring-up (`src/z80/main.s`)** — kernel and user heaps are
   initialized, the clock and keyboard timers are installed, the `"yos"` and
   `"gpx"` services are registered, `shell.sys` is loaded from the current
   esxDOS drive and started as a process.
3. **Scheduler activation** — IM2 is selected with the vector word at
   `0x5EFF` pointing at `__thread_robin`; every 50 Hz tick saves the current
   context, reclaims terminated threads and processes, chains timers, wakes
   waiting threads, selects the next runnable thread and restores its context.
4. **Runtime API** — there are no privilege levels; applications obtain the
   `yos_t` function table with `query_service("yos")` through `RST 0x18` and
   call the kernel, drivers, POSIX-style esxDOS filesystem and XPRG loader
   through pointers. `query_service("gpx")` returns the libgpx drawing API.

## Core Design Principles

- **RAM mutability over ROM immutability**: restart vectors in ROM jump through writable RAM entries; the scheduler hooks IM2 rather than the firmware-owned RST 38.
- **Owner-based resource model**: every kernel object and heap block carries an owner so a process's events, timers, services, image and stacks are reaped automatically.
- **Separation of OS and application memory**: kernel objects come from the 1 KiB `__sys_heap`, everything else from `__heap`.
- **Interrupt-time heartbeat**: scheduler, timers, keyboard scan and clock all derive from the 50 Hz frame interrupt.
- **Self-contained ROM**: no libc, no X runtime, no platform archive; applications link what they need into their own XL image.

## Documentation

Application authors should start with
[Programming YOS](docs/books/PROGRAMMING-YOS.md). Kernel contributors should
start with [The Book of YOS](docs/books/THE-BOOK-OF-YOS.md), which gives the
memory map and maps each subsystem to its source modules.

Release notes are in [CHANGELOG.md](CHANGELOG.md).

[language.url]:   https://en.wikipedia.org/wiki/Zilog_Z80
[language.badge]: https://img.shields.io/badge/language-z80%20asm-blue.svg

[standard.url]:   https://github.com/retro-vault/xyz/blob/main/y/include/yos.h
[standard.badge]: https://img.shields.io/badge/yos%20abi-9-blue.svg

[status.badge]:  https://img.shields.io/badge/status-development-red.svg
