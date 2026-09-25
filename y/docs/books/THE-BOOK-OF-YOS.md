# The Book of YOS

*yos* is a preemptive, ROM-based operating system for the ZX Spectrum 48K,
128K, and Spectrum Next. It is written entirely in hand-written Z80 assembly,
boots from a 16 KB replacement ROM, and cooperates with esxDOS on a divIDE
interface for disk access. Every process runs under a 50 Hz interrupt-driven
round-robin scheduler, and every kernel facility is reached through named
service tables rather than a fixed set of restart addresses.

This book is the entry point to the YOS documentation. Read it top to bottom
the first time; afterwards, jump straight to the chapter you need from the
table below.

If you want to write a C application rather than change the kernel itself,
start with [Programming YOS](PROGRAMMING-YOS.md) instead. That book covers
the XCC backend, the C library, services, files, input, graphics, packaging,
and every public API call.

## Chapters

Read top to bottom the first time; each chapter builds on the one before it.

| # | Chapter | Covers |
|---|---|---|
| 1 | [Boot](the-book-of-yos/BOOT.md) | reset, ROM header, vector table, IM2, critical sections |
| 2 | [Resource Accounting](the-book-of-yos/RESOURCE-ACCOUNTING.md) | lists, `sysobj_t`, ownership, `so_create` / `so_destroy` |
| 3 | [Memory Management](the-book-of-yos/MEMORY-MANAGEMENT.md) | heaps, block header, allocate / free / free-by-owner |
| 4 | [Banked Processes and Libraries](the-book-of-yos/BANKING.md) | logical banks, far calls, backend mapping |
| 5 | [Threads](the-book-of-yos/THREADS.md) | thread object, states, startup stub, context switch, events |
| 6 | [Processes](the-book-of-yos/PROCESSES.md) | process object, no-parent model, `process_start`, `load_process`, `process_exit` |
| 7 | [Libraries](the-book-of-yos/LIBRARIES.md) | shared/private libraries, initializer ownership, reference cleanup |
| 8 | [Cleanup and Resources](the-book-of-yos/CLEANUP-RESOURCES.md) | what the scheduler reclaims, and when |
| 9 | [Syscalls](the-book-of-yos/SYSCALLS.md) | services, RST 18, the unified `yos_t` table |
| 10 | [Clock](the-book-of-yos/CLOCK.md) | tick counters, timer chain, callback rules |
| 11 | [Program Images](the-book-of-yos/PROGRAM-IMAGES.md) | XPRG descriptor, loader checks, building `shell.sys` |

The [legacy README snapshot](the-book-of-yos/LEGACY-README-SNAPSHOT.md) is
the original, unedited README of the earlier C-era kernel, kept purely for
history. Where it disagrees with the chapters above, trust the chapters.

## Where things live

| Path | What it is |
|---|---|
| `y/src/z80/` | the assembly kernel that builds `yos-kernel.rom` (this book) |
| `y/src/c/` | the earlier C-and-assembly kernel, still buildable as `yos.rom`; it keeps its own copy of the old chapters under `y/src/c/docs/` |
| `y/include/` | `yos.h` and `yos.inc` define the public OS ABI; other headers describe separate services or legacy components |
| `y/tests/` | kernel emulation tests (`kernel-z80/`), the disk-resident shell/library fixture (`shell-yos/`), the real-esxDOS Fuse runner (`fuse/`), microdrive and tape harnesses |
| `y/pkg/` | optional host-tool sources (`appmake`, `microdrive`, `serial`), not part of the YOS distribution |
| `y/docs/books/` | this book; its chapters live under `y/docs/books/the-book-of-yos/` |
| `y/docs/standards/` | the [YOS assembly style guide](../standards/YOS-ASSEMBLY-STYLE-GUIDE.md) |

Build the ROM with `make -C y` from the repository root, or `make -C y/src/z80`
for the assembly kernel alone, and run the emulated kernel tests with
`make -C y/src/z80 test`. Model-labelled binaries land in `bin/y/arch/48/`,
`bin/y/arch/128/`, and `bin/y/arch/next/`; shared headers, firmware, scripts,
and source samples appear once, directly under `bin/y/`.

## The system in one page

1. **Reset.** The CPU starts at `0x0000` in `startup/crt0rom.s`. The first
   256 bytes are a firmware-compatible header: esxDOS re-enters at `0x0001`,
   RST 08 and NMI belong to esxDOS, RST 10 is an immediate `RET`, RST 18–30
   jump through a writable RAM table, and RST 38 gives the esxDOS-compatible
   IM1 return. YOS proper begins at `0x0100`.
2. **RAM bring-up.** `__startup_init` zeroes BSS and copies the eight-entry
   restart-vector table and the initialized data image from ROM into RAM.
   The 152-byte public service table, `__yos`, stays immutable in ROM.
3. **Kernel init (`main.s`).** The kernel creates the fixed OS heap and
   every configured 16 KiB bank arena, exposes the serialized system font in
   ROM without consuming heap memory, installs the clock, keyboard, and mouse
   timers, registers the
   `"yos"` service, loads `shell.sys` from the current esxDOS drive as an
   XPRG process, points RST 18 at the service lookup, and finally arms IM2
   with the scheduler vector at `0x5EFF`.
4. **Run.** Every 50 Hz frame interrupt enters `__thread_robin`, which saves
   the current thread's 22-byte register context and mapped bank, reclaims
   terminated threads and processes, chains timers, wakes threads whose
   events fired, selects the next runnable thread, and restores its context.
   Between interrupts the kernel itself idles in a `HALT` loop.
5. **Talk to the kernel.** YOS has no privilege levels. Applications call
   `query_service("yos")` through RST 18 and get back a `yos_t` table of
   function pointers — ABI version 6 — covering memory, timers, events,
   threads, processes, services, interrupt vectors, keyboard, mouse, a
   POSIX-style esxDOS filesystem, the shared XPRG process/library loader,
   and the complete libgpx drawing API.

Every shared-state transaction the kernel exposes uses nestable,
IFF-preserving critical sections. Kernel errno and loader status are
per-thread, stored in spare bytes of the 38-byte thread object. GPX creates
process-owned contexts and protects framebuffer updates, but it does not give
each application a private screen. See
[Concurrency](programming-yos/MEMORY-TIME-AND-CONCURRENCY.md) for the precise
guarantees, including the one remaining gap: linked libc `errno` is still
process-local rather than thread-local.

## Memory map

| Address | Region | Notes |
|---|---|---|
| `0x0000` | ROM header: reset, RST/NMI | esxDOS-compatible, 256 bytes |
| `0x0100` | ROM: kernel, drivers, fs, gpx, initializer images | `_CODE` .. `_GSFINAL`; ends below `0x4000` |
| `0x4000` | Screen bitmap and attributes | ULA |
| `0x5B00` | Writable kernel state | esxDOS descriptors/gates, clock, input, bank state |
| `0x5B37` | Kernel stack | 512 bytes, grows down from `0x5D37` |
| `0x5D37` | `__sys_vec_tbl`, list roots, mouse | Eight 3-byte `JP` entries |
| `0x5EFF` | `__im2_vector` | 2 bytes, read via `I=0x5E` |
| `0x5F01` | `__sys_heap` / `__heap` | One fixed OS heap, ending at `0xBFFF` |
| `0xC000` | Selected user heap | Banked XPRG processes, libraries and user allocations |
| `0xFFFF` | End of selected bank | |

Application authors should reach for the practical
[loadable-library chapter](programming-yos/LOADABLE-LIBRARIES.md) in
[Programming YOS](PROGRAMMING-YOS.md); the kernel-side layout of those images
belongs to this book's libraries chapter instead. For the complete banking
map, allocator, gate, and compiler contract, see
[Banked Processes and Libraries](the-book-of-yos/BANKING.md).

## Source map

Where to look in `y/src/z80/` when a chapter mentions a routine:

| Subsystem | Modules |
|---|---|
| reset and vectors | `startup/crt0rom.s`, `startup/_startup_init.s`, `startup/_sys_vectors.s`, `startup/sys_vec_get.s`, `startup/sys_vec_set.s`, `startup/_im2_init.s` |
| critical sections | `startup/enter_critical_section.s`, `startup/leave_critical_section.s`, `startup/_critical_state.s` |
| kernel init | `main.s`, `startup/_kernel_memory.s`, `kernel/_syscall_table_init.s`, `kernel/_yos_state.s` |
| lists and objects | `kernel/list_*.s`, `kernel/so_create.s`, `kernel/so_destroy.s` |
| memory and banking | `kernel/mem_*.s`, `bank/common/*.s`, `bank/{48,128,next}/_bank_map.s` |
| threads | `kernel/thread_create.s`, `kernel/_thread_prepare_startup.s`, `kernel/thread_resume.s`, `kernel/thread_suspend.s`, `kernel/thread_exit.s`, `kernel/_thread_lswitch.s`, `kernel/_thread_robin.s`, `kernel/_thread_select_next.s`, `kernel/_thread_cleanup_terminated.s`, `kernel/_thread_state.s` |
| processes | `kernel/process_start.s`, `kernel/process_exit.s`, `kernel/process_reap.s`, `kernel/process_load.s`, `kernel/boot_shell.s`, `kernel/_process_*.s` |
| events and timers | `kernel/evt_*.s`, `kernel/tmr_*.s`, `kernel/_tmr_chain.s` |
| libraries | `kernel/library_load.s`, `kernel/_image_*.s`, `kernel/_library_*.s`, `kernel/_so_reap.s` |
| services | `kernel/svc_register.s`, `kernel/svc_unregister.s`, `kernel/_svc_query.s`, `kernel/svc_query_rst18.s`, `kernel/_yos_*.s` |
| clock, keyboard, mouse | `drivers/clock.s`, `drivers/_clock_tick.s`, `drivers/keyboard_read.s`, `drivers/_keyboard_scan.s`, `drivers/*mouse*.s` |
| esxDOS filesystem | `fs/*.s` (`open`, `read`, `write`, `lseek`, `stat`, `opendir`, `readdir`, `enumerate_disks`, ... and the `_esxdos_*` gates) |
| graphics | `gpx/*.s` — vendored libgpx `v1.1.0-1-g0ef6f07` plus the `_gpx_name.s` / `_gpx_service.s` integration modules (see `gpx/README.md`) |
| link layout | `linker.lk` — `_HEADER` at 0, `_CODE` at 0x0100, `_DATA` at 0x5B00, `_IM2` at 0x5EFF, `_HEAP` at 0x5F01, reserved divIDE and Interface 1 trap addresses |

## Conventions used in the chapters

- Every kernel routine uses the `sdcccall(1)` calling convention: the first
  16-bit argument arrives in `HL`, the second in `DE`, further arguments on
  the stack, and 16-bit results return in `DE`. Chapters give C prototypes
  for readability, but the implementation behind each one is assembly.
- Struct layouts appear as C `typedef`s annotated with the byte offsets the
  assembly actually uses. The kernel has no C headers of its own — the
  public contract is `y/include/yos.h`.
- "Owner" always means the `process_t *` (or `thread_t *`) stored in a
  system object's header; `NONE` means a null owner, which is to say the
  kernel itself. A process object's own null owner does not mean "parent" —
  YOS stores no parent-process relationship at all.
