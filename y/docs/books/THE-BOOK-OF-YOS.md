# The Book of YOS

*yos* is a preemptive, ROM-based operating system for the 48K ZX Spectrum.
It is written entirely in hand-written Z80 assembly, boots from a 16 KB
replacement ROM, cooperates with esxDOS on a divIDE interface for disk
access, and runs every process through a 50 Hz interrupt-driven round-robin
scheduler and named service tables.

This book is the entry point to the YOS documentation. Read it top to bottom
the first time; afterwards jump to a chapter from the list beside the page.

If you want to write a C application rather than change the kernel, begin
with [Programming YOS](PROGRAMMING-YOS.md). It covers the XCC backend, libc,
services, files, input, graphics, packaging, and every public API call.

## Where things live

| Path | What it is |
|---|---|
| `y/src/z80/` | the assembly kernel that builds `yos-kernel.rom` (this book) |
| `y/src/c/` | the earlier C-and-assembly kernel, still buildable as `yos.rom`; it keeps its own copy of the old chapters under `y/src/c/docs/` |
| `y/include/` | public headers shared with applications: `yos.h`, `gpx.h`, `dirent.h`, `microdrive/microdrive.h` |
| `y/tests/` | kernel emulation tests (`kernel-z80/`), the disk-resident shell/library fixture (`shell-yos/`), the real-esxDOS Fuse runner (`fuse/`), microdrive and tape harnesses |
| `y/pkg/` | host tools staged into `bin/y/bin/`: `appmake`, `microdrive`, `serial` |
| `y/docs/books/` | this book; its chapters are in `y/docs/books/the-book-of-yos/` |
| `y/docs/standards/` | the [YOS assembly style guide](../standards/YOS-ASSEMBLY-STYLE-GUIDE.md) |

Build the ROM with `make -C y` from the repository root (or `make -C y/src/z80`
for the assembly kernel alone) and run the emulated kernel tests with
`make -C y/src/z80 test`. Output lands in `bin/y/z80/spectrum/bin/`:
`yos-kernel.rom`, the `shell.sys` process image it loads at boot, and the
`shelllib.svc` library used by that shell.

## The system in one page

1. **Reset.** The CPU starts at `0x0000` in `startup/crt0rom.s`. The first
   256 bytes are a firmware-compatible header: esxDOS re-enters at `0x0001`,
   RST 08 and NMI belong to esxDOS, RST 10 is an immediate `RET`, RST 18-30
   jump through a writable RAM table, and RST 38 is the esxDOS-compatible
   IM1 return. YOS proper begins at `0x0100`.
2. **RAM bring-up.** `__startup_init` zeroes BSS, copies the eight-entry
   restart-vector table and the initialized data image from ROM to RAM, and
   fills in the 96-byte public service table `__yos`.
3. **Kernel init (`main.s`).** Two heaps are created, the clock, keyboard and
   mouse timers are installed, the `"yos"` and `"gpx"` services are registered,
   `shell.sys` is loaded from the current esxDOS drive as an XPRG process,
   RST 18 is pointed at the service lookup, and finally IM2 is armed with the
   scheduler vector at `0x5EFF`.
4. **Run.** Every 50 Hz frame interrupt enters `__thread_robin`, which saves
   the current thread's 22-byte register context on its own stack, reclaims
   terminated threads and processes, chains timers, wakes threads whose
   events fired, selects the next runnable thread and restores its context.
   The kernel itself idles in a `HALT` loop.
5. **Talk to the kernel.** There are no privilege levels. Applications call
   `query_service("yos")` through RST 18 and receive a `yos_t` table of
   function pointers (ABI version 1): memory, timers, events, threads,
   processes, services, interrupt vectors, keyboard, mouse, a POSIX-style
   esxDOS filesystem, and the shared XPRG process/library loader. `query_service("gpx")`
   returns the complete libgpx drawing API.

Public shared-state transactions use nestable, IFF-preserving critical
sections. Kernel errno and loader status are per-thread, using spare bytes
in the 24-byte thread object. GPX creates process-owned contexts and protects
framebuffer updates; it does not give each app a private screen. See
[Concurrency](programming-yos/MEMORY-TIME-AND-CONCURRENCY.md) for the precise
guarantees, including the remaining process-local libc `errno` limitation.

## Memory map

| Address | Region | Notes |
|---|---|---|
| `0x0000` | ROM header: reset, RST/NMI | esxDOS-compatible, 256 bytes |
| `0x0100` | ROM: kernel, drivers, fs, gpx, initializer images | `_CODE` .. `_GSFINAL`; ends below `0x4000` |
| `0x4000` | Screen bitmap and attributes | ULA |
| `0x5B00` | `_INITIALIZED` | esxDOS gates at `0x5B37`, clock, kbd |
| `0x5B70` | `_BSS` | Descriptor, error, and timer state |
| `0x5B94` | `__yos` service table, then kernel stack | Table is 96 bytes; stack is 512 bytes, top at `0x5DF4` |
| `0x5DF4` | `__sys_vec_tbl`, list roots, mouse | Eight 3-byte `JP` entries |
| `0x5EFF` | `__im2_vector` | 2 bytes, read via `I=0x5E` |
| `0x5F01` | `__sys_heap` | 1024 bytes, kernel objects |
| `0x6301` | `__heap` | User heap to top of RAM |
| `0xFFFF` | End of RAM | |

Application authors should use the practical
[loadable-library chapter](programming-yos/LOADABLE-LIBRARIES.md) in
[Programming YOS](PROGRAMMING-YOS.md); the kernel-side layouts for those
images are in this book's libraries chapter.

## Appendix

- [Legacy README snapshot](the-book-of-yos/LEGACY-README-SNAPSHOT.md) — the original
  monolithic README of the C-era kernel, preserved verbatim. It describes
  the pre-esxDOS design (RST 38 scheduler, C sources) and is kept for
  history only; where it disagrees with the chapters above, the chapters
  are right.

## Source map

Where to look in `y/src/z80/` when a chapter mentions a routine:

| Subsystem | Modules |
|---|---|
| reset and vectors | `startup/crt0rom.s`, `startup/_startup_init.s`, `startup/_sys_vectors.s`, `startup/sys_vec_get.s`, `startup/sys_vec_set.s`, `startup/_im2_init.s` |
| critical sections | `startup/enter_critical_section.s`, `startup/leave_critical_section.s`, `startup/_critical_state.s` |
| kernel init | `main.s`, `startup/_kernel_memory.s`, `kernel/_syscall_table_init.s`, `kernel/_yos_state.s` |
| lists and objects | `kernel/list_*.s`, `kernel/so_create.s`, `kernel/so_destroy.s` |
| memory | `kernel/mem_init.s`, `kernel/mem_allocate.s`, `kernel/mem_free.s`, `kernel/mem_free_owner.s`, `kernel/_mem_payload_address.s` |
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

- All kernel routines use the `sdcccall(1)` calling convention: the first
  16-bit argument arrives in `HL`, the second in `DE`, further arguments on
  the stack, and 16-bit results return in `DE`. Chapters show C prototypes
  for readability; the implementation behind each is assembly.
- Struct layouts are given as C `typedef`s with the byte offsets the
  assembly actually uses. The kernel has no C headers of its own; the
  public contract is `y/include/yos.h`.
- "Owner" always means the `process_t *` (or `thread_t *`) stored in a
  system object's header, and `NONE` means a null owner, that is the kernel.
  A process object's own null owner does not mean "parent": YOS stores no
  parent-process relationship.
