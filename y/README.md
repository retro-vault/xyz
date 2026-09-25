![status.badge] [![language.badge]][language.url] [![standard.badge]][standard.url]

# YOS

`yos` is a preemptive, ROM-based operating system for the ZX Spectrum 48K,
128K, and Spectrum Next, written entirely in hand-written Z80 assembly. It boots from a 16 KiB
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
| `include/` | `yos.h` and `yos.inc` define the complete unified kernel ABI 1 |
| `samples/` | C and assembly `shell.sys` examples staged with source and binaries into the release tree |
| `third_party/esxdos089/` | esxDOS 0.8.9 DivIDE/DivMMC runtime and original notices staged into the Spectrum release |
| `pkg/` | optional host-tool sources: [`appmake`](pkg/appmake/README.md), [`microdrive`](pkg/microdrive/README.md), [`serial`](pkg/serial/README.md); they are not part of the YOS distribution |
| `tests/` | `kernel-z80/` emulated kernel test, `shell-yos/` library fixture, [`fuse/`](tests/fuse/README.md) real-firmware runner, `hello-yos/` and `mdr*-yos/` apps, [`mdr-emu/`](tests/mdr-emu/README.md) microdrive harness, and media |
| `docs/books/` | [Programming YOS](docs/books/PROGRAMMING-YOS.md) for application authors and [The Book of YOS](docs/books/THE-BOOK-OF-YOS.md) for kernel internals |
| `docs/standards/` | [YOS assembly style guide](docs/standards/YOS-ASSEMBLY-STYLE-GUIDE.md) |

## Build and Test

```bash
make -C y                  # current universal kernel and distribution
make -C y/src/z80          # same focused build -> bin/y/
make -C y/src/c            # historical C-era kernel, when explicitly needed
make -C y/src/z80 test     # boot the ROM under libxz80 and exercise the kernel
python3 y/tests/zesarux-next/run.py --headless
```

The assembly kernel is built with `xas`/`xld`/`xar` from `bin/x/bin`;
`shell.sys` is the minimal C sample: it draws `Hello World!` at screen centre
without loading a library or allocating heap memory, then loops forever. It is compiled as a relocatable XL application by the XCC `yos`
backend and packaged with `xprog`. The distribution at `bin/y/` includes the
matching esxDOS runtime; run its
`run-48.sh`, `run-128.sh`, or `run-next.sh` directly. Details, including how
to validate the ROM against real esxDOS, are in [AGENTS.md](AGENTS.md).

To show the shell in Fuse with real esxDOS firmware:

```sh
python3 y/tests/fuse/run.py
```

The runner uses `y/third_party/esxdos089/` by default; `--esxdos` remains an
override. See the [Fuse runner guide](tests/fuse/README.md) for dependencies
and cold-boot details.
For Spectrum Next/TBBlue validation with ZEsarUX, use the
[Next runner](tests/zesarux-next/README.md).
Application code for loadable libraries is covered in
[Loadable Libraries](docs/books/programming-yos/LOADABLE-LIBRARIES.md).

## System Overview

1. **Boot (`src/z80/startup/crt0rom.s`)** — the CPU starts at `0x0000`; the
   first 256 bytes are an esxDOS-compatible header (RST 08 and NMI belong to
   the firmware, RST 10 is an immediate `RET`, RST 18-30 jump through a
   writable RAM table). `__startup_init` zeroes BSS, copies the vector table
   and initialized data from ROM to RAM; the 152-byte `yos_t` table remains immutable in ROM.
2. **Kernel bring-up (`src/z80/main.s`)** — kernel and user heaps are
   initialized, the clock, keyboard and mouse timers are installed, the single `"yos"` interface is registered, `shell.sys` is loaded from the current
   esxDOS drive and started as a process.
3. **Scheduler activation** — IM2 is selected with the vector word at
   `0x5EFF` pointing at `__thread_robin`; every 50 Hz tick saves the current
   context, reclaims terminated threads and processes, chains timers, wakes
   waiting threads, selects the next runnable thread and restores its context.
4. **Runtime API** — there are no privilege levels; applications obtain the
   `yos_t` function table with `query_service("yos")` through `RST 0x18` and
   call the kernel, drivers, POSIX-style esxDOS filesystem, XPRG loader, and
   graphics implementation through that one table.

## Core Design Principles

- **RAM mutability over ROM immutability**: restart vectors in ROM jump through writable RAM entries; the scheduler hooks IM2 rather than the firmware-owned RST 38.
- **Owner-based resource model**: every kernel object and heap block carries
  an owner so process allocations, services, images, stacks, and library
  references can be reaped. Public timers remain explicitly managed.
- **Independent processes**: processes share an address space but do not have
  a parent/child relation, wait status, or exit-status channel.
- **Separation of OS and application memory**: kernel objects come from the one fixed OS heap (`__sys_heap`, aliased as `__heap`, spanning `0x5F01`-`0xBFFF`); process, library and application memory comes from a separate packed heap per logical bank at `0xC000`-`0xFFFF`.
- **Interrupt-time heartbeat**: scheduler, timers, keyboard scan and clock all derive from the 50 Hz frame interrupt.
- **Protected shared state**: syscall transactions use IFF-preserving critical
  sections, kernel errors follow the thread, and GPX contexts belong to apps.
  See [Concurrency](docs/books/programming-yos/MEMORY-TIME-AND-CONCURRENCY.md)
  for the remaining caller-owned state and libc `errno` limitations.
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
[standard.badge]: https://img.shields.io/badge/yos%20abi-6-blue.svg

[status.badge]:  https://img.shields.io/badge/status-development-red.svg
