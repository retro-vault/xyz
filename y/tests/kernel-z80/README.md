# YOS Kernel Regression

Run from the repository root:

```sh
make -C y/src/z80 test
```

The owning Makefile builds all artifacts under `build/yos-z80/` and
`bin/y/arch/48/`. Besides kernel, graphics and scheduler tests,
the harness executes the actual `shell.sys` produced by XCC/XLD/XPROG and
the build-only `shelllib.svc` regression fixture.

The harness uses the exact production ROM and its map. During reset only,
it defers `_boot_shell` at the CPU boundary until the disk fixture is ready;
it does not patch or separately link a test ROM.

`mock_filesystem.h` intercepts the initialized esxDOS **RAM gates**, not
the loader or its POSIX wrappers. It simulates short reads, native errors
and firmware register clobbers. Every intercepted gate must reject an IM2
interrupt, and success/error returns must preserve enclosing critical
sections. The shell loads the library and calls its
self-registered, relocated interface. An instruction checkpoint verifies
that registration is still private while the initializer runs.

`test_libraries.h` covers two clients, repeated acquisitions, independent
private instances, different ABIs, surviving sibling threads, last-client
cleanup, malformed descriptors/XL/exports, CRC errors, failed initialization,
and exhausted image/object/reference heaps.
It also interrupts a real load with IM2, runs a second loading thread to its
BUSY return, and resumes the first loader to successful initialization.

`test_thread_safety.h` audits shared-state writes and framebuffer reads/writes
under preemption, including descriptor reservation, append seek/write, event
and timer publication, timer-driven mouse sampling/snapshots, pixels, spans,
bitmaps and sprites.
The main harness checks IFF/flag preservation with nested sections and disabled
interrupt callers, per-thread errno/loader status, independent GPX contexts,
and the grouped ABI 6 ROM, including rejection of incompatible older images.
It executes `wait_event`
from a real thread, with another runnable thread and with all threads blocked,
then checks absent signals, pre-set signals and repeated timer wakeups.
Unsignalled waiters remain off the runnable queue; consumed signals reset.

Banking coverage verifies the fixed OS heap below `0xC000`, a separate user
heap in every configured bank, first-fit allocation across banks, and
current-bank-only allocation for ordinary near `malloc`. It also covers
packed library far tables, bank-owned image cleanup, RST 20 calls and jumps,
RST 28 dynamic calls, RST 30 data access, register and stack-argument
preservation, four nested cross-bank frames and checked overflow. The
the dedicated library fixture performs a real three-argument far call and exercises its standard `malloc`/`free` wrappers. The 128K and
Next test variants emulate their physical banking hardware and inspect every
configured bank. Interrupt-driven round-robin coverage also requires the
scheduler to restore saved logical banks 0 and 1 exactly. Boot verifies that
the ROM-packed system font expands
byte-accurately into fixed RAM and still renders through GPX.

`test_circles.h` compares every framebuffer byte against a host midpoint
reference for 528 outline/fill, radius, clipping, edge-position and blit-mode
configurations. XOR cases also draw twice and require an empty framebuffer.
`test_rom_layout.py` verifies that the ROM patcher rejects reserved-range
overlaps, including all-zero live data, damaged firmware slots, overlapping
areas and overflow of the 16 KiB ROM boundary. Failed checks must leave
the input ROM untouched. Test scratch files stay under the build directory.

This is deterministic ROM-level coverage, not real esxDOS firmware or
hardware validation. Use the [Fuse cold-boot runner](../fuse/README.md) for
visual validation with actual esxDOS. The shell should display centred `Hello World!`
below its greeting.
