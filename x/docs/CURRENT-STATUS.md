# Current Status & Handoff

This document captures the state of the project as of the most recent major work session, so that future sessions (human or AI) can quickly get back up to speed.

Last updated: 2026-08-09, after the large-program XCC size pass.

## Major Recent Work

### Large-program `-Os` and non-loadable data

Large generated assemblies no longer lose the ordinary bounded peephole pass
at the old 8,000-line cliff: the scalable guard is now 16,000 lines.  Size
mode also gets four bounded tail-merge/outline fixed-point rounds and a small,
outline-safe final layout cleanup.  The backend keeps cheap adjacent
compare/branch and switch lowering active when it disables expensive
whole-function matching, and size-profitable dense byte jump tables are no
longer limited to an arbitrary 16-entry span.  Selection uses local byte-cost,
type, span, and control-flow proofs only.

`xld` now preserves the object format's `never_load`/NOBITS property.  BSS
continues to occupy runtime memory and define its address/length symbols, while
BIN and IHX occupancy excludes it.  This removes trailing startup-cleared BSS
from CP/M COM files without changing the linked address map; explicit binary
ranges and holes between loadable areas keep their zero fill.

Static numeric pointer initializers now preserve their absolute values, with
typed pointer arithmetic applied before integer fallback.  Qualified arrays of
forward-declared records also remain bound to the canonical tag definition.
This fixes the large-program case where `extern const struct T table[]` was
declared before `struct T` was completed and dynamic indexing advanced by one
byte instead of `sizeof(struct T)`.

### 0. XCC Optimization Guardrails And Empty Experimental Speed Lane

The exact whole-function `try_emit_sdcc_style_helper` selector was removed.
Its structural recipes were derived while repeatedly measuring the z88dk
corpus; matching a complete function by IR shape is benchmark recognition
even without filename checks. The pending fixed-global interprocedural
specialization was also removed. Twenty-seven stale exact-assembly assertions
tied to the removed recipes were retired, but their manifests and executable
correctness coverage remain.

The repaired scalar-local promotion and physical-home allocator have now
graduated into the validated `-Of` speed profile.  The source-independent
dense-dispatch profitability guard follows scalar promotion there.  The
adjacent 32-bit conditional-shift recurrence rewrite graduated into both
`-Of` and `-Os`, because avoiding intermediate spills is strictly faster and
smaller.  `-O3` is intentionally empty again: it is an exact alias of `-Of`
with a distinct profile identity reserved for the next experiment.  Four
unrelated assembly probes and every completed aggregate benchmark report
byte/cycle-identical `-Of` and `-O3` output.  Pure size policy remains
exclusive to `-Os`.
The subsequent holdout audit repaired two general proof gaps: lockstep pointer
walking rejects copied indices with competing natural-loop definitions, and
xopt limits IX self-store cleanup to compiler-described temporary-frame slots
and preserves the accumulator-defining half unless all paths overwrite `A`
before reading it. Source-local slots remain untouched when volatility
metadata is unavailable. Focused executable and assembly regressions cover
both cases.
An additional ABI1 defect found by the full matrix was fixed: compact-frame
comparison lowering could treat a reserved spill slot for an incoming register
parameter as initialized stack storage and leave a frameless function's SP
unbalanced.
Static pointer initializers were also repaired: object, array-element,
member, and function addresses now emit relocatable symbol expressions with
addends, and external symbols referenced only by initializer data are declared
to the assembler.  Compile and execution regressions cover both internal and
external addresses, including the pattern that broke Lunatik's glyph table.

The 2026-07-29 frozen baseline executed 8301 compiler variants with zero failures:
ABI0 has 2684 compile and 1477 execution passes; ABI1 has 2688 compile and
1452 execution passes. The four matrices also contain 73 manifest-declared
opposite-ABI skips. The expanded default E2E entrypoint now runs the runtime,
libc, ABI-split compiler matrices, C23 execution, external corpora, z88dk
compatibility suite, remaining host tools, CP/M and YOS applications, and
all MDR modes instead of silently omitting those phases.
The current unified XCC-filtered manifest run passes 4,229/4,229 variants.

All compiler lanes in the 23-program comparison now use the same `z80_exec`
timing model. Every XCC lane passes 23/23. The strong result is linked-image
size: `-Os` is strictly smaller than the best successful competitor on 22/23.
The Pareto-safe recurrence graduation also raises `-Os` to 3/23 speed wins
against SDCC and 1/23 strict speed wins against the competitor envelope.
The graduated `-Of` result, and therefore the empty `-O3` alias, remains
correct on 23/23 in both ABIs and is strict-fastest against the successful
competitor envelope on 13/23 programs; it wins 14/23 against SDCC.  Its ABI1
geometric-mean cycle count is 0.65% below SDCC but 9.89% above the per-program
best successful competitor envelope (ABI0: 9.72% above).  Detailed
measurements are in `x/tests/benchmarks/z88dk24/RESULTS.md`.

Every lane in the frozen 40-program portable corpus passes.  `-Of` and `-O3`
are identical at 27,194 payload bytes and 2,676,535 cycles, but remain 69.22%
slower than the fastest competitor and win no individual program.  Every XCC
profile passes the more varied 20-program bare-metal holdout; `-Of` and `-O3`
are identical there too at 13,813 bytes and 4,617,452 cycles.  The numeric
holdout passes 50/50.  Accordingly, no general speed-lead claim is made.

The current active manifest expands to 4,262 compiler variants.  The complete
set passed in bounded category/range shards after the graduation, as did the
standalone xopt smoke suite and `make -C x`.  The four benchmark suites are
consolidated under `build/x/benchmarks/profiles-promoted-final/`.

Only Pareto-safe size transforms were promoted to speed mode: redundant
pair-immediate reload removal, two-/four-byte `push af` stack allocation, and
unused temporary-frame compaction through four bytes. Slower outlining,
tail-merging, and larger push-sequence transforms remain size-only.

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

Headers were updated in `x/libc/include/` for the new declarations.

### 2. Testing — "Tests for All Functions. Both."
A very large dual-style test base was built:

- **Direct tests** (library + runtime isolation): heavy use of `runtime_machine.hpp` calling symbols directly from the assembled image (`rt_sym::...`). Large data-driven arrays, edge cases (0/-0/±Inf/NaN, overflow, INT_MIN, etc.), bit-exact or tolerance comparisons against host gcc.
- **C-driven tests** (compiler + library together): `.c` files compiled with the project's `xcc`, assembled, linked into the test image (alongside the full libc + runtime + "none" sys hooks), then executed in the emulator. The harness inspects return codes in registers and/or captured output via the `__sys_putchar_*` hooks.
- Runtime-specific matrices (`x/tests/runtime/`) for long long and double (activated the `PENDING_TEST` suites and added mega cross-product tests).
- The external C23 compatibility suite was copied into `x/tests/c23/` and a representative/enriched version was integrated into the in-tree dispatch (`x/tests/libc/c23_cases.c`).

This dual approach was explicitly requested so that later runs can help distinguish "problem in the library" vs. "problem in the compiler".

### 3. C23 Compatibility Suite Integration
The comprehensive external C23 test suite (originally at `/home/tstih/data/tstih/c23`) was copied into `x/tests/c23/`. 
- It provides ~63 feature tests across all categories (core-language, library, time, iec-60559/fromfp+minmax, unicode/char8_t, initialization/structs, stdckdint, stdbit, free_sized, etc.).
- An `xcc-z80` profile + driver/run scripts were added so the suite's matrix runner can be used against this project's toolchain.
- The in-tree `c23_cases.c` was enriched with logic transcribed from the suite, ensuring "all structures" (both C structs like `div_t`/`timespec`/etc. and the full set of test categories) are exercised when running the normal `make -C x/tests/libc core-test`.

### 4. Repo Structure & Distribution Discussion
The project was recognized as becoming complex (toolchain + libc + runtime + full OS + future GUI + tests + packaging all in one tree). The user explicitly wants:
- The ability to build and publish the "x tools" (xcc, xas, xld, ...) as an **independent** distributable.
- Tests that are primarily local to the component that owns the code.
- Still support genuine end-to-end tests that cross components.

A full restructuring proposal was developed (see `docs/ARCHITECTURE.md` for the detailed target layout, migration steps, and rationale).

### 5. Prefix-Rooted XtTools Staging
The xtools staging layout has now started moving toward a real standalone
compiler-suite install tree:

- `make -C x` builds the standalone compiler suite without requiring the
  whole OS build; the root Makefile no longer exposes the old `xtools` alias.
- Output is now split into `bin/x/`, `bin/y/`, and `bin/z/`.
- `bin/x/` is the xtools install prefix:
  - `bin/x/bin/` for host executables
  - `bin/x/include/` and `bin/x/lib/` for host SDK headers and libraries,
    including the shared `libxopt` assembly optimizer plus debugger/emulator
    support libraries such as `libxgdb`, `libxemu`, `librsp`, and `libxz80`
  - `bin/x/z80/include/` for staged target libc headers
  - `bin/x/z80/lib/` for `crt0`, linker scripts, `libruntime.a`, `libc.a`,
    the default platform archive (`libnone.a`), and named payloads such as
    `libcpm3.a`
- `bin/y/` carries YOS outputs plus YOS-adjacent host tools such as
  `appmake`, `microdrive`, `serial`, and `libmicrodrive.a`.
- `bin/z/` carries staged target assets such as Spectrum app payloads and
  `.mdr` media.
- `xcc` now probes its install prefix for default headers, and `xld` probes
  its install prefix for default runtime/startup archives.
- The debugger target split now treats `xemu` as its own xtools product:
  a standalone emulator executable plus reusable `libxemu` for host-side
  execution testing.
- Z80 assembly peepholes now live in shared `lib/xopt`; `xcc` links
  `libxopt.a`, and the standalone `xopt` tool can optimize `.s` files
  directly.
- The copied runtime `.rel` staging tree was removed from the public install
  layout; runtime helpers are now shipped as `libruntime.a`.

### 6. Native Root Build for YOS
The default top-level build flow now leans fully on the staged X toolchain:

- The root `Makefile` no longer hard-blocks non-Linux hosts and no longer
  folds packaging into the default `make` path.
- `make` at the repo root now builds X first, then builds Y natively using the
  staged `bin/x/bin/xcc`, `xas`, and `xld`.
- The active YOS ROM build in `y/src/Makefile` now compiles C sources with
  `xcc --mode=sdcc -S -Os`, assembles with `xas`, and links with `xld`
  against staged `libc.a` and `libruntime.a`.
- The Y-side sample app builds under `y/tests/hello-yos/`,
  `y/tests/mdrsave-yos/`, `y/tests/mdrtst-yos/`, and `y/tests/mdrstep-yos/`
  were migrated off the Docker/SDCC path to the same staged X toolchain flow.
- A small YOS compatibility pass was needed so the older SDCC-oriented kernel
  and driver sources also build cleanly under `xcc`/`xas`:
  `[[sdcc::naked]]` attributes, a few inline-asm bridges, and symbol-name
  alignment between C and assembler entry points.

## Current High-Level Layout (Pre-Restructuring)

- `src/xc/` — the x tools (xcc, xas, xld, xopt, ...)
- `lib/libc/` — the assembler C library
- `x/runtime/` + related — low-level runtime
- `src/yos/` — the OS
- `x/tests/` and `y/tests/` — migrated test suites (libc, runtime, e2e helpers, benchmarks, debug, media, and the C23 matrix)
- `lib/` — supporting libs (xz80, xbfd, sys layers, etc.)
- Root `Makefile` orchestrates via `SUBDIRS`
- Packaging in `pkg/`, outputs in `bin/` / `build/`

## Open / Next Steps (from the Structure Discussion)

- Continue the incremental product-root migration and add a root `xtools`
  convenience alias only if it remains useful; `make -C x` is canonical now.
- Create dedicated packaging for the standalone xtools product.
- Flesh out the `toolchain/tests/`, `libc/tests/`, etc. ownership once directories move.
- Decide on sysroot / target layout for the distributable xtools.
- Keep reducing assumptions that require a specifically Linux-hosted developer
  environment; the current build is native across GNU Make + POSIX-like shells,
  but Windows still expects something like MSYS2 rather than pure cmd.exe.
- Keep evolving the dual-style test base as new C23 or OS features are added.
- The copied `x/tests/c23/` suite should remain the authoritative source for the broad C23 matrix; the in-tree dispatch is for fast local verification + libc surface testing.

## How to Resume Work Here

1. Read (in this order):
   - `AGENTS.md` (root)
   - `x/docs/ARCHITECTURE.md`
   - `x/docs/CURRENT-STATUS.md` (this file)
   - Component READMEs (`lib/libc/README.md` or equivalent, `src/yos/README.md`, etc.)

2. The session system will also feed a compacted history when you return to this directory. The documents above are the durable, human-readable memory.

3. Common entry points:
   - `make -C x`
   - `make -C x/tests/libc core-test` (the main libc + C23 dispatch runner)
   - `cd x/tests/c23 && make matrix PROFILE=...` for the full external suite against the current xcc profile

Feel free to ask the AI (or a future human) to re-read these three documents + the relevant source trees at the start of any new session on this project.

## Recent Artifacts Worth Knowing About

- `x/tests/libc/c23_cases.c` — enriched in-tree C23 test (covers all major categories + structs from the external suite + our specific libc additions).
- `x/tests/c23/` — the full copied C23 compatibility suite + xcc-z80 integration.
- Large test data in `x/tests/libc/test_main.cpp`, `stdio_cases.c`, `x/tests/runtime/test_*.cpp`.
- All the new C23 assembler implementations live in the existing files under `lib/libc/src/` (math/moremath*.s, stdlib/strtod*.s + heap_core, stdio/printf.s, etc.).

Update this file (and the architecture doc) when significant new work is completed or when the restructuring actually begins.
