# Y Changelog

This changelog is reconstructed from the tagged source trees and build layout
for `y`, the YOS operating system.

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release for the repository as a whole.

## Unreleased

- Extended the public kernel interface to ABI version 8 with an XPRG process
  loader and its error cell. The ROM now finds `shell.sys` on the current
  esxDOS drive, validates its 64-byte process descriptor, OS requirement,
  payload CRC and embedded XL relocation records, allocates the declared
  application stack plus scheduler context, starts the process, and transfers
  ownership of the resident image. The build creates a temporary XCC-compiled
  process image with `xprog`; the initial screen greeting/footer has been
  removed. The complete boot loader path occupies 884 ROM bytes.
- Extended the public kernel interface to ABI version 7 with bounded esxDOS
  directory and disk enumeration. Applications can now use `opendir`,
  `readdir`, `rewinddir`, and `closedir`; each returned `dirent` contains the
  short 8.3 name, file size, native attributes, and `DT_REG` or `DT_DIR`.
  `enumerate_disks` safely probes physical devices into a caller-sized array
  instead of using esxDOS's unbounded whole-device-list operation. Directory
  state is allocated from the caller heap and the ROM implementation remains
  independent of libc and platform archives.
- Integrated the complete ZX Spectrum libgpx v1.1.0 implementation into the
  assembly kernel ROM and registered its 23-function direct-call table as the
  named `"gpx"` service. The new public `gpx.h` describes the drawing context,
  bitmap, font, sprite, geometry, constants, and complete `gpx_api_t` service
  ABI. The vendored graphics modules have no libc or runtime dependency; their
  eight writable bytes use the kernel's ROM-to-RAM initializer path.
- Replaced the legacy public `yos_t` surface with ABI version 6, containing
  only kernel services and the complete esxDOS-backed POSIX file interface.
  Descriptive names such as `create_thread` now map directly to the existing
  assembly implementations. Only memory allocation, memory release, and timer
  creation retain the small adapters required to hide kernel-private owner or
  heap arguments. The ROM owns its filesystem implementation, descriptor
  state, error cell, and 45-byte RAM gate block; it has no dependency on a ZX
  platform, libc, runtime, console, or font archive. The public
  `error_number` pointer exposes the ROM's error cell without an accessor.
- Added self-contained ZX Spectrum keyboard and Kempston mouse drivers to the
  kernel ROM. The keyboard matrix is scanned by a 50 Hz kernel timer into a
  transition queue, while mouse calibration and polling read the three
  Kempston ports directly. ABI v6 exposes `read_key`, `calibrate_mouse`, and
  `read_mouse` as direct function pointers without proxy routines. Removed
  the redundant `query_interface` compatibility spelling; applications use
  `query_service` for the RST 18 lookup.
- Split the assembly-only kernel into one callable global function per source
  module, with underscore-prefixed modules for internal helpers and shared
  state. The production link now extracts these modules from
  `libyos-kernel.lib`, allowing xld to omit unused kernel APIs. The emulator
  validates every physical RST and NMI entry address. RST 38 keeps the exact
  divIDE/esxDOS-compatible `PUSH AF`, `POP AF`, `EI`, `RETI` sequence. YOS
  preserves its 50 Hz preemptive scheduler through a two-byte IM2 vector at
  `0x5EFF`, installed only after the disk loader has completed.
  Reset, RST 8 and NMI now also preserve divIDE's delayed first-opcode
  protocol. A single readable CRT header uses ordinary origins and zero bytes
  for all fixed vectors; no per-vector linker areas remain. YOS leaves the
  reserved tails of RST 8 and NMI NOP-filled rather than installing handlers
  there; the fixed esxDOS base-ROM byte reader is present at `0x007B`.
  RST 10 remains the immediate return required by esxDOS boot text; named
  YOS service lookup is exposed to RAM processes through RST 18.
- Made the YOS microdrive test images depend explicitly on the staged
  `bin/y/bin/microdrive` host tool. The application and emulator test
  Makefiles now build that tool with explicit Y product paths, so an inherited
  X staging prefix cannot redirect or omit it during the unified test run.

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
