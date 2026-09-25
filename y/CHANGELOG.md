# Y Changelog

This changelog is reconstructed from the tagged source trees and build layout
for `y`, the YOS operating system.

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release for the repository as a whole.

## Unreleased

- Kept YOS at pre-release ABI 1 and regrouped its compact 76-entry,
  152-byte `yos_t` table. `get_sys_info` now sits with `version` and
  `rom_model` and exposes live heap, process, thread, timer, event, service,
  and library-reference roots through public read-only layouts. System-list
  object owners are packed far pointers rather than address-only IDs.

- Load final `.sys` paths case-insensitively into the fixed OS heap with bank
  `FFh`; ordinary processes and libraries remain banked. The loader shrinks
  XL metadata and relocation records before allocating process/thread state.
  Regression coverage loads a 19 KiB system image that compacts to 14 KiB
  without changing any banked heap. With the system font restored to ROM, the
  clean-boot staging ceiling is 24,804 bytes on disk, or 24,193 bytes for a
  512-byte-stack image that reclaims no
  relocation tail.

- Reworked the deployed C and assembly shells into allocation-free, library-free
  `Hello World!` samples centred through the unified `yos_t` graphics API.
  The library image remains a build-only regression fixture.

- Merged the retained GPX types, constants, and 22 drawing calls into the end
  of the single `yos_t` ABI 1 table; removed polygon drawing/filling and the
  separate `gpx` service and header.

- Flattened the deployable release from `bin/y/z80/spectrum/` to `bin/y/`.
  Shared headers, firmware, scripts, notices, and sample source now occur
  once; `arch/48/`, `arch/128/`, and `arch/next/` contain only model-labelled
  binary payloads. The layout audit rejects obsolete trees and binaries
  outside those three directories.

- Consolidated the core interface around `yos.h`/`yos.inc`: directory handles,
  entries, limits, and entry-kind constants now live in `yos.h`, and disk
  enumeration is exposed only through `yos_t`. Removed the duplicate global
  `enumerate_disks` wrapper and the XCC-only `yos_set_putchar_hook` adapter
  from the public API, and stopped shipping the POSIX adapter header and
  separate GPX service header in the core YOS distribution.

- Restored `shell.sys` as the boot-process contract throughout the kernel,
  build, tests and documentation. The default C shell and a minimal assembly
  shell are now shipped as source and built XPRG samples. `bin/y` stages a
  self-contained Spectrum release with ABI 1 C/assembly headers and 48K,
  128K, and native Next ZEsarUX launchers. The matching esxDOS 0.8.9 DivIDE
  and DivMMC runtime is bundled with its original notices, so no external
  firmware path is required. Build audits and other test-only artifacts
  remain under `build/`.

- Added a ZEsarUX TBBlue banking runner using the production universal ROM.
  Its headless mode combines the all-126-bank libxz80 backend test with a real
  NextReg/MMU probe in ZEsarUX that far-calls from bank 0 to bank 125 and
  verifies restoration. It clones its private FAT16 HDF to raw IDE media, then
  verifies real 48K and 128K divIDE/esxDOS cold boots through the resident
  process, library reference, and rendered shell. The 128K run also requires
  model detection and all six user banks.
- Fixed the 128K mapper to retain the 48-BASIC ROM slot from which ESXIDE
  boots YOS. Fixed the scheduler restoring saved logical bank `n` as `n+1`;
  an interrupt-driven regression now checks exact restoration of banks 0 and
  1, and the real 128K test exercises the fix through banked process/library
  execution.

- Reorganized the `yos_t` service table into contiguous ABI 1
  categories: identity, banked memory, time/critical sections, timers/events,
  threads, processes/libraries, services, vectors, input, filesystem, and
  commands/console. Every entry in both public `yos.h` copies now has a
  concise contract comment. Added matching `yos.inc` files with named byte
  offsets for all functions and data-pointer slots, converted assembly
  consumers from magic offsets, and added a build-time checker that compares
  both C headers, both assembly includes, and the ROM table. Current examples
  and fixtures use `--min-os 1`. Identity starts with `version`, `rom_model`,
  `get_sys_info`, and `set_print_hook`; `rom_model` reports 48K, 128K, or
  Next. The unified table is 152 bytes.

- Added banked YOS process and library images. One fixed OS heap contains all
  kernel objects and stacks below `0xC000`; every detected logical bank has
  an independent packed 16 KiB user heap at
  `0xC000-0xFFFF`. Boot probes exact NextReg 0 machine IDs first, then
  performs a reversible 7FFD page test, and otherwise selects 48K. It records
  the model and installs a three-byte JP to the matching mapper in fixed RAM.
  A 48K machine uses one logical no-op bank, Spectrum 128K uses at most six
  safe 7FFD pages, and Spectrum Next uses the configured count of paired-MMU
  banks while excluding the two fixed lower-RAM pages.
  Process/library ownership, loader rollback,
  shrinking, reference counting, and reaping now include bank extents.
  Public allocation scans all user banks and returns a packed far pointer;
  standard libc `malloc` deliberately allocates only from the caller's
  execution bank so a near pointer remains valid. Library interfaces are
  stable fixed-memory `{bank,address}` tables.
  RST20 implements inline far call and bit-7 far jump, RST28 implements XCC
  indirect far calls, RST30 implements compiler far-data reads and writes,
  and the scheduler saves the exact interrupted bank. Four checked
  fixed-memory call frames per thread preserve nested calls and
  callee-cleaned stack arguments. XCC far pointers now use the shared packed
  `bank,lo,hi` ABI and keep the bank invariant under arithmetic.
  The full 48K emulator suite covers calls, jumps, register/stack behavior,
  allocation/rollback/lifetime, standard `malloc`, and a real three-argument
  XCC library call. Hardware-modelled maximum-count 128K and Next variants
  exercise every configured bank. The universal ROM ends at `0x3F75`, leaving
  139 contiguous bytes; the layout guard reserves the final 128 bytes.

- Restored the immutable 1,440-byte proportional system font directly to ROM.
  GPX returns its ROM address without a boot-time expansion or fixed-heap
  allocation, recovering 1,447 usable OS-heap bytes including allocator
  overhead.

- Second ROM-size pass removes another 87 linked code/data bytes and
  recovers 73 usable tail bytes: content now ends at `0x3EFC`, leaving
  **260 contiguous zero-filled bytes**, including the complete final page.
  Outline/fill circles share midpoint arithmetic; library export binding
  uses register pointers and ownership transfer returns its resident pointer.
  The complete boot filename is packed beside its caller, not removed.
  Fonts/cursors, service slots, RAM addresses and esxDOS reservations remain
  intact. Map-based build checks reject any code/data overlapping fixed slots
  (even zero-valued data) or entering the last page. Regression tests cover
  those rejection paths and 528 clipped/unclipped circle configurations.

- Reduced linked ROM code/data by 201 bytes against the pre-pass build,
  sharing IX-frame entry/return sequences and esxDOS gate selection,
  replacing indexed status copies with pointer walks, and simplifying owned
  object cleanup. Repacking around fixed slots moves the occupied end from
  `0x3FFF` to `0x3F45`: 187 contiguous zero-filled bytes, 186 newly recovered.
  Hardware traps, print/interrupt slots, vector image and RAM layout remain
  intact; no banking is implemented. Builds and tests check the zero-filled
  tail. Status tests cover all 256 attribute bytes and signed-size limits.

- Packed ABI 3 into the 16 KiB ROM using short branches and checked fixed
  slots for the print entry, interrupt returns, service name and vector image.
  The build validates those slots and emits a SHA-256 file for the final ROM.

- Added ABI 3 `exec_command(commandline)` and `set_print_hook(sink)` at `yos_t` slots 50 and 51 (bytes 100 and 102), backed by esxDOS `M_EXECCMD` ($8F). Dot-command output through RST 10 and the fixed 09F4h print entry is routed to the temporary RAM sink while the firmware call masks scheduler interrupts. The boot-time print path remains inert until RAM gates are initialized.

- Added ABI 2 `wait_event(event)` at `yos_t` slot 49 (byte 98), preserving
  the existing 49 slots. A wait publishes the caller on the scheduler's
  waiting queue with a stack-resident event handle and no allocation. The
  interrupt scan atomically consumes one binary signal for one waiter;
  repeated sets coalesce. With no runnable threads the scheduler now idles
  on the kernel stack instead of restoring a blocked thread. Timer hooks
  only signal; threads wake to do the work. Call waits with interrupts
  enabled, outside critical sections, and keep the event alive. Kernel tests
  execute the public wait with and without another runnable thread, repeated
  timer signals and a signal set before waiting. ABI 1 images still load.
  ROM packing uses header and pre-trap gaps; content ends at `0x3FFF`.

- An earlier unreleased iteration temporarily changed the boot-process
  filename. The public contract is now `shell.sys`. Ordinary process images
  use the `.prc` extension; `.svc` remains reserved for libraries and
  registered services.

- Added `shrink_memory(memory, size)` to the public `yos_t` table (slot 48,
  appended after the ABI 1 baseline; the table is now 98 bytes). It releases
  the bytes of a live `allocate_memory` block beyond `size` when they can form
  a heap block, never moves the block, and returns `NULL` for a header that
  is not allocated. `__image_retain` now uses the same routine to drop a
  consumed XL relocation table from its read buffer and leaves the metadata
  prefix to the loader's ordinary final cleanup. `__mem_split` also treats a
  retained size beyond the block as "no split". Kernel tests exercise the
  public slot: in-place trim, tail reuse, unsplittable remainders, oversized
  requests and freed blocks. ROM content ends at `0x3fef`, leaving 17 bytes.

- Adopted XL image format version 2, in which the relocation table follows
  the code instead of preceding it. The loader still reads one XPRG payload
  into a single block and relocates in place, but `__image_retain` now splits
  the consumed relocation table off the *end* of that block and frees it
  before splitting off the metadata prefix, so only code (plus the compact
  service table) stays allocated and the freed table coalesces with the space
  above it. `__process_relocate` validates the version 2 layout and returns
  the code size; `__mem_split` now owns the remainder-size threshold shared
  by allocation and retention. Kernel tests check the exact resident block
  for both the shell process and the library.

- The shared process/library loader now relocates inside its original read
  buffer. After validating and binding exports, it splits off and frees the
  leading JP/XL/relocation metadata, retaining only code/static storage and
  compact service pointers. This removes the simultaneous second code copy
  that prevented TED's textedit service from loading alongside File Manager.
  `_mem_split.s` is shared with normal allocation; the header/owner layout,
  XPRG/XL format and public ABI are unchanged. Kernel validation, rollback,
  shared-library lifetime and concurrent-loading tests pass. ROM content ends
  at `0x3fe7`, leaving 25 bytes inside the 16 KiB limit.

- Fixed real esxDOS 0.8.9 directory conversion: native records contain the
  attributes byte before the ASCIIZ short name, followed by date/time and
  size. Tests now reproduce the actual firmware layout.
- Added optional `A:`/`B:` path prefixes in the shared filesystem adapter.
  These are YOS/application conventions, translated to native drives `0x40`
  (DivIDE master) and `0x48` (slave), with the prefix removed before firmware
  entry. Unqualified paths retain the current-drive behavior. The same adapter
  serves directory reads and process/library file loading. The table ABI stays
  unchanged and the ROM content still ends at `0x3fe0`.

- Fixed simultaneous keyboard transitions in a matrix row: the scan loop
  restored its bit countdown and then subtracted the row offset a second
  time, turning overlapping letters into unrelated key codes. The correction
  removes one ROM byte. Tests cover press and release chords in all eight rows.

- Process and library loading now retains only relocated code/static storage
  (plus the compact two-byte export table for services). XPRG descriptors, JP
  metadata, XL headers and relocation records live in a temporary allocation
  that is freed after relocation. Startup now uses BSS for zero state,
  generates the RAM esxDOS gates, and publishes the immutable YOS ABI table
  directly from ROM. Immutable syscall/gate-selector data fills the unused
  `0x0080..0x00f2` header region; the entry remains `0x0100` and ROM content
  ends at `0x3fe0`, inside the 16 KiB boundary. Kernel tests cover exact
  resident allocation and metadata cleanup.

- Fixed the NMOS Z80 `LD A,I` interrupt race at critical-section entry.
  IM2 repairs the saved parity flag at that exact instruction boundary, so
  frequent graphics/input syscalls cannot silently disable the scheduler.
  Added matching/nonmatching-PC flag-preservation tests. ROM link roots and
  member order pack the repair around the existing paging holes; ABI 1 and
  all reserved addresses are unchanged.

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
  named `"gpx"` service. The initial public `gpx.h` described the drawing context and the then-separate
  graphics service ABI; ABI 6 later moved all of those definitions and calls
  into `yos.h` and `yos_t`. The vendored graphics modules have no libc or runtime dependency; their
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
