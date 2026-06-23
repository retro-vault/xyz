# Y Changelog

This changelog is reconstructed from the tagged source trees and build layout
for `y`, the YOS operating system.

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release for the repository as a whole.

## v1.7.1 - Beta - 2026-06-20

- No `y` code or build changes were identified relative to `v1.7.0`.
- YOS remained the same ROM-based ZX Spectrum operating system tree with apps,
  drivers, kernel, startup, and TTY directories.

## v1.7.0 - Alpha - 2026-06-20

- No `y` code or build changes were identified relative to `v1.6.0`.

## v1.6.0 - Alpha - 2026-06-20

- No `y` code or build changes were identified relative to `v1.5.0`.

## v1.5.0 - Alpha - 2026-06-20

- Moved YOS staged output under the dedicated `bin/y` prefix.
- Updated the process loader relocator in `y/src/kernel/_process_relocate.s`
  to understand flagged one-byte relocations, including high-byte-only patch
  handling.

## v1.4.0 - Alpha - 2026-06-07

- Retargeted the YOS build from the earlier `xlink`-based flow to `xld`.
- Added `libxbfd` as a YOS build dependency through the updated `src/yos/Makefile`.
- Staged ROM output under the newer `z80/spectrum/bin` layout and enabled
  `xld -g` output during the YOS build.
- Preserved the existing YOS subsystem structure: apps, drivers, kernel,
  startup, TTY, and the chapter-based OS documentation.

## v1.3.0 - Alpha - 2026-05-24

- No `y` code or build changes were identified relative to `v1.2.0`.

## v1.2.0 - Alpha - 2026-05-23

- Tightened the YOS ROM link protection around the ZX Spectrum Interface 1 ROM
  paging trigger by reserving `0x1708` specifically in the build flags.
- Updated the YOS build to compile `xlink` with the shared debugger include
  path and embedded `lib/xdbg` source support.
- Kept the same YOS subsystem and documentation layout introduced in the
  initial tag.

## v1.1.0 - Alpha - 2026-05-17

- No `y` code or build changes were identified relative to `v1.0.0`.

## v1.0.0 - Alpha - 2026-05-17

- Initial tagged YOS baseline with apps, drivers, kernel, startup, include,
  docs, and TTY subsystems already present.
- Documented YOS as a preemptive ROM-based ZX Spectrum operating system with
  interrupt-driven scheduling, service-table syscalls, dual heaps, timers,
  processes, and threads.
- Shipped chapter-based operating system documentation under `y/docs/`,
  covering boot, resource accounting, memory management, threads, processes,
  syscalls, timers, cleanup, and application images.
