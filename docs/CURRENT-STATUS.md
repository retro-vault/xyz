# Current Status & Handoff

This document captures the state of the project as of the most recent major work session, so that future sessions (human or AI) can quickly get back up to speed.

Last updated: during the C23 completion + repo structure discussion.

## Major Recent Work

### 1. C23 Standard Library Completion (Libc in Assembler)
The primary focus for a long period was bringing the hand-written Z80 assembler libc up to a full (or very close to full) C23 surface, strictly following the project's rules:
- Only edit **existing** `.s` files (no new source files for implementation).
- Pure assembler for the library functions (headers allowed for declarations and macros).
- Thread-safety: no new writable static data / `_DATA` variables. Everything uses stack, registers, or deliberate library state.
- Style consistency with the rest of the existing libc (`.module`, `.optsdcc -mz80 --sdcccall(1)`, IX frames, EXX for 64-bit, common cores like `strtod_core`, `transf_core`, `sfp_`, etc.).

Key C23 additions implemented:
- `strfromd` / `strfromf` / `strfroml` (real digit generation using the double runtime + stack frames) + wiring into `printf` for `%f`/`%a`/etc. and basic `%b` support.
- Full `fromfp*` / `ufromfp*` / `fromfpx*` / `ufromfpx*` family (all 8 variants, all three precisions) + `roundeven*`.
- Complete `fmaximum*` / `fminimum*` family (all magnitude/num variants for float/double/long double).
- `getpayload*`, `setpayload*`, `setpayloadsig*`, `totalorder*`, `totalordermag*`.
- `free_sized` / `free_aligned`.
- `timespec_getres`.
- `char8_t` + `mbrtoc8` / `c8rtomb` (added to the uchar layer).
- Real `stdckdint.h` macros + assembler overflow helpers (`__ckd_add_sint` etc.).
- Various supporting pieces (more math wrappers, string extensions like `strverscmp`/`basename`/`dirname` in existing files, buffering improvements, etc.).

Headers were updated in `lib/libc/include/` for the new declarations.

### 2. Testing — "Tests for All Functions. Both."
A very large dual-style test base was built:

- **Direct tests** (library + runtime isolation): heavy use of `runtime_machine.hpp` calling symbols directly from the assembled image (`rt_sym::...`). Large data-driven arrays, edge cases (0/-0/±Inf/NaN, overflow, INT_MIN, etc.), bit-exact or tolerance comparisons against host gcc.
- **C-driven tests** (compiler + library together): `.c` files compiled with the project's `xcc`, assembled, linked into the test image (alongside the full libc + runtime + "none" sys hooks), then executed in the emulator. The harness inspects return codes in registers and/or captured output via the `__sys_putchar_*` hooks.
- Runtime-specific matrices (`tests/runtime/`) for long long and double (activated the `PENDING_TEST` suites and added mega cross-product tests).
- The external C23 compatibility suite was copied into `tests/c23/` and a representative/enriched version was integrated into the in-tree dispatch (`tests/libc/c23_cases.c`).

This dual approach was explicitly requested so that later runs can help distinguish "problem in the library" vs. "problem in the compiler".

### 3. C23 Compatibility Suite Integration
The comprehensive external C23 test suite (originally at `/home/tstih/data/tstih/c23`) was copied into `tests/c23/`. 
- It provides ~63 feature tests across all categories (core-language, library, time, iec-60559/fromfp+minmax, unicode/char8_t, initialization/structs, stdckdint, stdbit, free_sized, etc.).
- An `xcc-z80` profile + driver/run scripts were added so the suite's matrix runner can be used against this project's toolchain.
- The in-tree `c23_cases.c` was enriched with logic transcribed from the suite, ensuring "all structures" (both C structs like `div_t`/`timespec`/etc. and the full set of test categories) are exercised when running the normal `make -C tests/libc core-test`.

### 4. Repo Structure & Distribution Discussion
The project was recognized as becoming complex (toolchain + libc + runtime + full OS + future GUI + tests + packaging all in one tree). The user explicitly wants:
- The ability to build and publish the "x tools" (xcc, xas, xld, ...) as an **independent** distributable.
- Tests that are primarily local to the component that owns the code.
- Still support genuine end-to-end tests that cross components.

A full restructuring proposal was developed (see `docs/ARCHITECTURE.md` for the detailed target layout, migration steps, and rationale).

## Current High-Level Layout (Pre-Restructuring)

- `src/xc/` — the x tools (xcc, xas, xld, ...)
- `lib/libc/` — the assembler C library
- `src/xc/xcc/lib/runtime/` + related — low-level runtime
- `src/yos/` — the OS
- `tests/` — mixed (libc tests, runtime tests, e2e, benchmarks, debug, and the new `tests/c23/`)
- `lib/` — supporting libs (xz80, xbfd, sys layers, etc.)
- Root `Makefile` orchestrates via `SUBDIRS`
- Packaging in `pkg/`, outputs in `bin/` / `build/`

## Open / Next Steps (from the Structure Discussion)

- Decide on exact migration order and start the incremental refactoring (introduce `make xtools`, move `src/xc` → `toolchain/`, move tests into component directories, etc.).
- Create dedicated packaging for the standalone xtools product.
- Flesh out the `toolchain/tests/`, `libc/tests/`, etc. ownership once directories move.
- Decide on sysroot / target layout for the distributable xtools.
- Keep evolving the dual-style test base as new C23 or OS features are added.
- The copied `tests/c23/` suite should remain the authoritative source for the broad C23 matrix; the in-tree dispatch is for fast local verification + libc surface testing.

## How to Resume Work Here

1. Read (in this order):
   - `AGENTS.md` (root)
   - `docs/ARCHITECTURE.md`
   - `docs/CURRENT-STATUS.md` (this file)
   - Component READMEs (`lib/libc/README.md` or equivalent, `src/yos/README.md`, etc.)

2. The session system will also feed a compacted history when you return to this directory. The documents above are the durable, human-readable memory.

3. Common entry points:
   - `make xtools` (once the target exists) or the current `make -C src/xc ...`
   - `make -C tests/libc core-test` (the main libc + C23 dispatch runner)
   - `cd tests/c23 && make matrix PROFILE=...` for the full external suite against the current xcc profile

Feel free to ask the AI (or a future human) to re-read these three documents + the relevant source trees at the start of any new session on this project.

## Recent Artifacts Worth Knowing About

- `tests/libc/c23_cases.c` — enriched in-tree C23 test (covers all major categories + structs from the external suite + our specific libc additions).
- `tests/c23/` — the full copied C23 compatibility suite + xcc-z80 integration.
- Large test data in `tests/libc/test_main.cpp`, `stdio_cases.c`, `tests/runtime/test_*.cpp`.
- All the new C23 assembler implementations live in the existing files under `lib/libc/src/` (math/moremath*.s, stdlib/strtod*.s + heap_core, stdio/printf.s, etc.).

Update this file (and the architecture doc) when significant new work is completed or when the restructuring actually begins.