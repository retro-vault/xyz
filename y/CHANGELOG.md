# Y Changelog

This changelog is reconstructed from the tagged source trees and build layout
for `y`, the YOS operating system.

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release for the repository as a whole.

## Unreleased

- Moved Kempston hardware sampling onto the kernel's 50 Hz timer chain beside
  clock and keyboard scanning. `read_mouse` now returns an atomic snapshot of
  bounded absolute coordinates instead of polling ports; button transitions
  accumulate until read. Shared setup installs all three boot timers without
  increasing that initialization code.
- Protected public kernel shared-state transactions: event/timer publication
  and removal, mouse state, descriptor reservation/validation/commit, append
  seek/write, and directory operations. Nestable critical sections now preserve
  flags and the outermost IFF state, including use inside timer callbacks.
  Kernel errno and loader status follow each thread using formerly unused
  thread-object bytes; object/context sizes remain 24/22 bytes.
- GPX now allocates independent six-byte process-owned contexts and frees them
  on destroy or process cleanup. Creation no longer clears another app's
  screen. Raster spans, bitmap rows, pixels and sprite operations protect
  shared framebuffer read/modify/write accesses; compound draws can interleave.
- Shared protected-call entry, shorter filesystem/vector helpers and ROM
  packing keep the production image at `0x3FF0`: 16 bytes remain, with fixed
  reserved addresses unchanged. Tests now execute that exact production ROM.
  Added forced two-thread loader contention, per-thread errors, shared-state
  access audits, GPX isolation and interrupt-state preservation regressions.
  Linked C-library `errno` remains process-local; the threading documentation
  distinguishes it from the kernel's per-thread error cells.
- Reset the unreleased assembly-kernel interface numbering to ABI 1. The
  complete 96-byte table, including process loading, process-owned allocation
  and services, and library loading, is now the clean compatibility baseline.
  GPX has its own 24-call service table. Both public headers, ROM validation, test images,
  examples, and documentation use minimum OS ABI 1.
- Fixed real-esxDOS shell/library boot: the common firmware adapter now
  holds a nestable critical section until divIDE restores the YOS ROM.
  Previously an IM2 interrupt during a disk call entered mapped firmware
  instead of the ROM scheduler and restarted YOS before drawing. Added
  interrupt-rejection/nesting regressions and a pristine Fuse boot runner
  under `tests/fuse/`. Verified the production shell's greeting and
  "Library OK" with Fuse 1.6.0 and esxDOS 0.8.9. At that stage the ROM ended
  at `0x3FBD`; the current post-thread-safety margin is reported above.
- Added ABI 1 `load_library(path, flags)`: private/shared XPRG services
  reuse the process loader's disk, CRC and XL relocation core. Library
  initializers run once after relocation, can self-register their relocated
  interface, and receive library-owned allocations and staged registration.
  Six-byte client references retain threadless library process objects until
  the last acquiring process thread exits. Failed loads roll back resources.
  Ordinary service registration is now process-owned and names are bounded.
- Reduced ROM use by removing repeated scheduler/creation frames, compacting
  heap splitting and exact reads, sharing object cleanup and eleven esxDOS
  register adapters, and shortening descriptor validation branches. Fixed
  the surviving-thread scan's NZ result so a sibling thread keeps its
  process and library references alive.
  The optimized existing routines save 439 code bytes in aggregate.
  With library loading and the updated GPX service, those changes initially
  left 67 bytes; later thread-safety work uses part of that space and the
  current ROM margin is reported above.
- Added real `shelllib.svc` packaging and a shell call/display check, plus
  emulator coverage for relocation before self-registration, staged
  publication, shared/private state, ABI identity, repeated acquisitions,
  sibling-thread lifetime, malformed images, short/error reads, initializer
  failure and image/object/reference allocation failure.

- Updated the vendored ZX Spectrum libgpx to upstream commit `0ef6f070`
  (`v1.1.0-1-g0ef6f07`). The optimized drawing core adds `BM_OR`, standard
  line-pattern constants, a resize cursor, and the selected-edge `draw_box`
  primitive. Its service pointer was appended as slot 24 so all existing
  service offsets remain stable. YOS-specific context allocation and raster
  synchronization adaptations are recorded in `src/z80/gpx/README.md`.

- Added the XCC `yos` application backend and made the assembly-kernel build
  reproduce `shell.sys` through it: XCC emits relocatable XL and XPROG wraps
  the process. The CRT initializes relocated C storage, obtains the current
  table through RST 18, and terminates through `exit_process`. Standard libc
  allocation and POSIX file/directory calls now delegate to YOS; allocations
  are charged to the current process. Console output remains invisible unless
  a process installs the new character hook for a future Alto console.
  Added a complete step-by-step Programming YOS book, full YOS/GPX call
  references, and a runnable toolchain sample. XPROG can also place `.sys`
  files on deterministic partitioned FAT16 esxDOS IDE images.

- Restructured the Y documentation. `y/docs/books/THE-BOOK-OF-YOS.md` is
  the entry point and its chapters live in `y/docs/books/the-book-of-yos/`, the assembly
  style guide under `y/docs/standards/`, and every chapter was rewritten
  against the assembly kernel in `y/src/z80/` (IM2 scheduling, `__startup_init`,
  the real memory map, the 24-byte thread and 15-byte process objects, the
  scheduler-driven `process_reap` cleanup, the `HL`/`DE` RST 18 convention,
  the 32-bit clock counters and the `ticks + 1` timer period, XPRG loader
  error codes). `y/INDEX.md` and `y/README-src-yos.md` were folded into
  `y/README.md` and a new `y/AGENTS.md`; the duplicate `y/CHANGELOG-y.md`
  was removed. Markdown files are now named `UPPER-CASE-WITH-HYPHENS.md`.
- Added an XPRG process loader and its error cell to the public kernel
  interface. The ROM now finds `shell.sys` on the current
  esxDOS drive, validates its 64-byte process descriptor, OS requirement,
  payload CRC and embedded XL relocation records, allocates the declared
  application stack plus scheduler context, starts the process, and transfers
  ownership of the resident image. The build creates a temporary XCC-compiled
  process image with `xprog`; the initial screen greeting/footer has been
  removed. The complete boot loader path occupies 884 ROM bytes.
- Added bounded esxDOS
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
  first integration used an eight-byte global context in the kernel's
  ROM-to-RAM initializer path; the current per-process contexts are described
  in the newer entry above.
- Replaced the legacy public `yos_t` surface with a kernel-only table containing
  only kernel services and the complete esxDOS-backed POSIX file interface.
  Descriptive names such as `create_thread` now map directly to the existing
  assembly implementations. Only memory allocation, memory release, and timer
  creation retain the small adapters required to hide kernel-private owner or
  heap arguments. The ROM owns its filesystem implementation and has no
  dependency on a ZX platform, libc, runtime, console, or font archive. That
  first filesystem surface used a 45-byte RAM gate block and exposed its error
  cell through the `error_number` pointer without an accessor; the current
  gate count and error virtualization are recorded above.
- Added self-contained ZX Spectrum keyboard and Kempston mouse drivers to the
  kernel ROM. The keyboard matrix is scanned by a 50 Hz kernel timer into a
  transition queue; at this stage mouse calibration and polling read the three
  Kempston ports directly. The earlier development table exposed `read_key`, `calibrate_mouse`, and
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
