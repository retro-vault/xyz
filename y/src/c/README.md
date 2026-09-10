![status.badge] [![language.badge]][language.url] [![standard.badge]][standard.url]

The YOS
=======

`yos` is a preemptive ROM-based operating system for ZX Spectrum (48K target), written in C and Z80 assembly.

It boots from ROM, initializes mutable runtime structures in RAM, and then runs the system entirely through interrupt-driven scheduling and service tables.

## Why This README Is Shorter Now

The old monolithic README was split into chapter files so each subsystem can be maintained and read independently.

Nothing was removed from documentation:

- detailed chapters are in `src/yos/docs/`
- a full snapshot of the original long README is preserved in:
  - [`docs/01_legacy_readme_snapshot.md`](docs/01_legacy_readme_snapshot.md)

## System Overview

`yos` works as a small cooperative kernel + preemptive thread runtime:

1. **Boot (`startup/crt0rom.s`)**
   - CPU starts at `0x0000`
   - startup code installs RAM-backed vector indirection table
   - global/static data is copied from ROM initializers to RAM
2. **Kernel Bring-up (`main.c`)**
   - system and user heaps are initialized
   - periodic timer hooks are installed (cursor, keyboard scan, clock)
   - OS service table (`"yos"`) is registered
   - shell process is started
3. **Scheduler Activation**
   - `RST 0x38` is hooked to `__thread_robin`
   - every 50 Hz tick saves current context, chains timers, selects next runnable thread, restores context
4. **Runtime API Model**
   - no privilege levels on Z80, so system calls are exposed as named services
   - programs resolve function tables (for example via `query_service("yos")`) and call through pointers

## Core Design Principles

- **RAM mutability over ROM immutability**: restart vectors in ROM jump through writable RAM entries.
- **Owner-based resource model**: kernel objects carry ownership metadata (`sysobj_t`) to support cleanup/accounting.
- **Separation of OS vs app memory**: OS objects come from `__sys_heap`, user allocations from `__heap`.
- **Interrupt-time heartbeat**: scheduler, timers, and clock derive from the 50 Hz interrupt.

## Important Practical Notes

- Timer callbacks run in interrupt context; keep them short and allocation-free.
- Thread stacks are per-thread and finite; stack sizing must include context-save overhead.
- Current code has known extension points (for example `_process_cleanup` is present and intended for fuller cascade cleanup).

## Full Documentation

Use the chapter index:

- [`INDEX.md`](INDEX.md)
- [`docs/11_app_format.md`](docs/11_app_format.md)

Application image note:

- current process loading still consumes raw relocatable `XL` images
- the new `.app` chapter defines the upcoming legacy ZX Spectrum container used by the host-side converter

[language.url]:   https://isocpp.org/
[language.badge]: https://img.shields.io/badge/language-c-blue.svg

[standard.url]:   https://en.wikipedia.org/wiki/C_(programming_language)
[standard.badge]: https://img.shields.io/badge/standard-c11-blue.svg

[status.badge]:  https://img.shields.io/badge/status-development-red.svg
