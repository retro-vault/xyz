# Current Status & Handoff

This document captures the state of the project as of the most recent major work session, so that future sessions (human or AI) can quickly get back up to speed.

Last updated: 2026-08-27, after completing and verifying the installable CPC
toolchain package surface.

## Major Recent Work

### Medium default model and the 24-program compiler comparison

Ordinary root, standalone-X, libc, and packaging builds now default to the
medium `X_MODEL=M` feature set: `float` and `long` remain available, while
source-level `double` and `long double` alias the configured `float` ABI.
Genuine 64-bit double, `long long`, and stdio floating conversions are
omitted. Unsuffixed floating literals therefore use float arithmetic in M;
double-spelled libc calls are redirected to the matching float entry points.
The small binary64 conversion subset retained internally by the float parser
is not exposed as the M source ABI. Explicit `X_MODEL=S|L` builds and the
side-by-side `x-s`, `x-m`, and `x-l` targets are unchanged.

The feature-filtered model matrices are run with `make test-x-models`. The
unfiltered E2E suite intentionally uses the staged `bin/x-l` compiler and
sysroot, so its double and long-long coverage remains exhaustive even though
the ordinary `bin/x` prefix is M. The C23 profile accepts that compiler path
as an override. YOS's shared minimal test CRT exports both the XCC and SDCC
names for its local HL/IY indirect-call trampolines, preventing the runtime
archive from being selected solely for an alias and then defining the same
routine twice.

The locked shared-z88dk suite now has 24 programs. Its added
`bitfieldbench` performs repeated packed-field extraction, insertion, and
read-modify-write operations against a host-verified checksum of 60004. XCC's
optimized failures were fixed by making spill-slot liveness follow the actual
CFG and by routing `IY`-derived pointer fusions through guarded pointer
lowering. The separate `fixedbench` failure was z88dk's stack-ABI
`___muluint2ulong` being selected for an XCC register-ABI call; the pinned
integration now selects a distinct 23-byte XCC archive member only when used.
XCC `-Os` and `-Of`, sccz80, and both 80cc configurations pass 24/24. Current
zsdcc returns 34924 for `bitfieldbench` and passes 23/24; the failure remains
visible in the saved matrix instead of being excluded.

The structural optimizer campaign added generic shift/mask fusion, proven
loop-reduction and dense-dispatch register residence, immutable byte-argument
residence in `E`, pair-pressure scheduling for simple pointer walks, direct
word-producer/store forwarding, and consecutive word-load/add scheduling.
The compiler does not inspect benchmark names, source text, constants, or
whole-program fingerprints. Against the best valid result from current
zsdcc, 80cc-fp, and 80cc-sp on each row, XCC `-Of` is strictly fastest on
13/24 programs and `-Os` on 7/24; both profiles remain correct on all 24.

XCC now has a first-class `--runtime=z88dk-classic` profile. Literal formats
across the printf/scanf families are summarized into z88dk's classic handler
masks, including flags, widths, precision, length modifiers, scansets, and
long-long printf conversions. zcc passes its per-link option file through
`-zcc-opt=<file>`, and XCC appends OR-able capability blocks from every
translation unit before the CRT is assembled. Dynamic formats and escaped
formatter addresses retain the supported classic fallback surface;
program-defined functions named printf or scanf are excluded. The benchmark
uses this public runtime integration rather than hard-coded masks. XCC `-Os`
is consequently the strict smallest valid image on 24/24 rows and `-Of` on
22/24, while the speed results above remain unchanged.

The reported unsigned-byte conditional-assignment reduction is not broken in
the current XCC. A permanent executable case tests changed, unchanged, and
255-to-zero wraparound paths and passes `-O0`, `-O1`, `-O2`, `-O3`, `-Of`,
and `-Os`.

Final validation after the optimizer campaign is clean. The feature-filtered
model matrices pass S 3,677/3,677, M 3,937/3,937, and L 4,382/4,382. The
exhaustive L-model compiler lanes pass 2,741 variants under ABI0 and 2,765
under ABI1, with 45 and 21 manifest-declared skips respectively; the execution
lanes pass 1,602 ABI0 variants and 1,577 ABI1 variants, with 10 and 35 skips.
The C23 matrix passes 59 claimed features, the fixed project corpus passes
22/22, the applicable algorithm corpus passes 51/51 plus 11 float-rich cases,
and z88dk compatibility passes 280/280. Runtime, libc, xz80, every host-tool
phase, CP/M, YOS application images, MDR modes, the full-chain integration,
the ZX Spectrum MCP RAM/ROM/TAP/TZX check, and platform-layout validation also
pass. The only failures in the superseded broad run were two ABI1 assembly
shape probes being executed once under ABI0; their manifests now declare ABI1
explicitly, and the complete compile phase passes after rerun.

### ZX Spectrum 48K RAM and ROM platforms

The staged X sysroot now contains `zx-ram` and `zx-rom` platform archives,
CRTs, and linker scripts. The RAM target begins at `0x5CCB`, initializes BSS
and static storage, supplies a heap up to `0xF000`, and uses the top 4 KiB for
stack growth. The replacement target produces a fixed 16 KiB ROM, places
writable state at `0x5B00`, and copies initialized `_DATA` from its packed ROM
load image before entering C. `xld` gained the underlying VMA/LMA support via
GNU `AT>region` and the SDCC-style `COPY area` directive, including generated
`s__AREA_LOAD`/`l__AREA_LOAD` symbols and ROM overflow diagnostics.

Both targets use assembly-only platform code. The public `<stdio.h>`
exports non-blocking `trygetchar()`; blocking standard input
is derived from the same matrix poller. Standard output/error render the
proportional 6x12 Tamsyn font exported by snatch. The screen addressing and
12-pixel scroll structure
are adapted from YOS. File and wall-clock hooks fail deliberately, and the
libc `time`, `clock`, and `timespec_get` wrappers now propagate those failures.
The long-text Fuse demo also exposed and fixed an exact-right-edge cursor
case: Z80 `INC` does not update carry, so an advance from pixel 255 to zero
must test the zero result explicitly to avoid overprinting the current row.

`xprog --tap` and `--tzx` produce checksummed, auto-running Spectrum images.
Their BASIC bootstrap invokes a small ROM loader placed after a `REM` token;
the final ROM load returns directly to the requested entry, so the CODE block
can overwrite the BASIC program and genuinely use the low `0x5CCB` address.
The optional `x/tests/tests/zx48/run_mcp.py` test builds and executes raw RAM,
replacement ROM, TAP, and TZX under zx-spectrum-mcp with a real 48K ROM. It
checks static initialization, BSS, heap/libc operations, unsupported hooks,
console scrolling, non-blocking polling and blocking keyboard input, exit, and
a final marker in every form.
The target-specific `x/examples/zx-ram/lorem.c` and
`x/examples/zx-rom/lorem.c` programs are built and visually verified in Fuse
as tape-loadable RAM code and a replacement ROM, respectively.

### Amstrad CPC 464, 664, and 6128 platforms

The staged sysroot now also contains self-contained `cpc-464`, `cpc-664`, and
`cpc-6128` platform archives, CRTs, and linker scripts. Each target loads at
`0x4000`, initializes C storage, exposes the linked-image-to-`0x9F00` heap,
uses a private stack below the firmware/AMSDOS reservation, retains firmware
interrupts, and returns from `main()` to BASIC. Console output uses the Text
VDU, blocking and non-blocking input use the Keyboard Manager, and the time
hooks convert the firmware's 300 Hz 32-bit ticker to the libc `timespec` ABI.

The cassette-only 464 supplies failing filesystem hooks without pulling any
AMSDOS workspace or buffers into the linked image. The 664 and 6128 use the
stock AMSDOS ROM in slot 7. Their CRT selects disk input/output and closes the
loader channel left open by BASIC `RUN`; direct ROM trampolines install the
foreground-ROM `IY` workspace before entering the AMSDOS CAS routines. The
backend exposes the firmware's one input and one output channel as descriptors
3 and 4 and supports raw and stdio read/write/close, input seeking,
rename/remove, and verified failure returns. Headerless ASCII files report no
length at open, so EOF establishes their length and `SEEK_END` measures them
through the ROM when necessary. Update/append output and output seeking fail
explicitly because AMSDOS output streams are sequential.

`xprog --cdt` writes CPC firmware header/data records in a CDT 1.20 container.
`xprog --dsk` writes a standard 40-track, single-sided CPCEMU DSK with an
AMSDOS data filesystem and one checksummed 8.3 binary. The optional
`x/tests/tests/cpc/run_mcp.py` regression boots the actual CDT on a CPC 464 and
independent writable DSK images on CPC 664 and CPC 6128 ROM sets. All three
models pass static/BSS/heap and common-libc checks, console polling/blocking
input, clock setting/reading, clean BASIC return behavior, and—on disk
models—raw descriptors plus `fopen`/`fread`/`fwrite`/`fseek`/`ftell`,
rename/remove, headerless seeking, and missing-file errors. Separate examples
under `x/examples/cpc-464`, `cpc-664`, and `cpc-6128` reproduce the supported
media workflows.

The optional root `make packages` pass now builds both the native Debian X
toolchain package and the XGDB VSIX. All CPC CRT objects and sources, GNU/SDCC
linker scripts, platform archives, the installed CPC guide, and `xprog` media
modes are explicit archive checks. Debian payload ownership is normalized to
`root:root`, executable/data modes are verified, and the finished `.deb` is
extracted before acceptance. The extracted package's own `xcc`, sysroot, and
`xprog` pass the three CPC MCP delivery runs (464 CDT and 664/6128 DSK).

### CP/M command-line startup

The CP/M 3 `crt0` now copies the bounded 127-byte command tail out of the
default DMA area and constructs a C-conforming `argc`/`argv` on the descending
stack. `argv[0]` is empty because CP/M supplies no program name,
`argv[argc]` is null, ASCII whitespace is collapsed, and double-quoted spans
form one argument with the quotes removed. Storage is sized to the actual tail
and argument count and remains valid when file I/O overwrites address `0x0080`.
The real CP/M emulator regression covers no arguments, ordinary and repeated
whitespace, quoted and empty-quoted words, DMA overwrite, the exact 127-byte
tail, and 62 total `argv` entries. It also checks `<stdio.h>`
`trygetchar()` against both idle and ready console input; the assembly backend uses
BDOS function 11 without consuming the waiting character. Startup also
transfers the `main` return value from DE to the HL argument expected by
`exit`. It supplies `argc` and
`argv` simultaneously through the `sdcccall(1)` HL/DE registers and the
right-to-left `sdcccall(0)` stack layout; the emulator suite exercises both
entry conventions.

### Parser-oriented optimizer work

The generic optimizer now covers several recurring parser shapes without
recognizing individual sources: word-index sentinel loops can become lockstep
pointer walks, direct call/zero-test/return chains stay in return registers,
and framed ABI1 wrappers with register-only terminal calls can become sibling
jumps. The speed profile additionally packs one private byte argument into an
otherwise idle A register and lowers the bundled C-locale ctype family inline.
Both speed-only choices remain outside `-Os`; ctype lowering is interposition-
aware within the translation unit and has an explicit `-fno-ctype-builtins`
escape hatch. Focused parser-shaped execution coverage runs at O0, Os, and Of.

Against the same current SQL sources compiled before and after the pass, final
`-Os` `_CODE` sizes changed by +56 bytes for `program.c`, -10 bytes for
`sql.c`, and -17 bytes for `optimize.c` (+29 bytes over the three units). The
conservative A-register correctness fix accounts for the small net increase.
`-Of` changed by +200, +211, and +34 bytes respectively; these are deliberate
speed-profile code-size trades, but no SQL runtime-cycle claim is made without
a target workload. On the unrelated frozen z88dk corpus, all 22 valid `-Os`
pre/post pairs and all 23 `-Of` pairs are exactly unchanged in bytes and
cycles. The remaining `hashbench -Os` row changes from an incorrect execution
to correct, giving final XCC correctness of 23/23 in both profiles. The full
post-fix XCC suite passes 4,382/4,382.

### S-model core integer closure

The S distribution retains the compiler's complete 32-bit integer runtime
(multiply, divide, modulo, widening, and shifts) even though its optional
`long` text/formatting surface remains disabled.  Core libc services whose
standard ABI happens to contain `long` are likewise present: file positioning,
the integer-only time/calendar family, `rand`/`srand`, and checked integer
multiplication.  All remain one-routine archive members, so unused services do
not increase linked program size.  `atoi` is now a standalone 16-bit parser
and no longer pulls in or depends on `strtol`.

The model-S suite directly executes these services under every optimization
profile and once again includes the existing 32-bit recurrence, deep-call,
clock/time ABI, and micro-Max integer workload regressions.

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
Byte-sized views of an `IY`-resident source local or temporary now use
`IYL`/`IYH` as well. Previously a byte compare could read the abandoned `IX`
spill slot even though the corresponding word update lived in `IY`; the
static-address-initializer holdout now passes under both ABIs and all five
active optimization profiles.
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

The 24-program full-z88dk comparison is now a separately reproducible,
seven-lane matrix. It pins an old z88dk target sysroot (the only baseline that
reproduces the supplied sccz80 table), current nightly zsdcc and host tools,
the latest active 80cc development head, XCC's M distribution, compiler rule
files, corpus content hash, and upstream `z88dk-ticks -b msx`. It measures
complete linked images and retains binaries, maps, logs, versions, and
executable hashes.

The final locked run has XCC `-Os` and `-Of` correct on 24/24. Current zsdcc
passes 23/24, its six expensive-allocation rows pass 6/6, and both latest 80cc
modes pass 24/24 after a rule-only missing-`EXTERN` fix. Against zsdcc alone,
XCC `-Of` wins 18/23 valid speed comparisons; against the stronger per-row
zsdcc/80cc envelope it is strictly fastest on 13/24. The previous runner's
raw XCC compatibility object contributed 38 bytes to every linked image; it
has been replaced by zero-byte aliases plus a separate lazy 23-byte multiply
ABI adapter. Automatic classic-runtime format selection also makes `-Os`
strictly smallest on 24/24 rows and `-Of` on 22/24. The report identifies the
old object's five bodies, explains the ABI fix and generic format inference,
lists each structural optimization, and preserves the exact binaries, maps,
hashes, and full comparison matrix in
`x/tests/benchmarks/z88dk24/RESULTS.md`.

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
- Runtime-specific matrices (`x/tests/tests/runtime/`) for long long and double (activated the `PENDING_TEST` suites and added mega cross-product tests).
- The external C23 compatibility suite was copied into `x/tests/tests/c23/` and a representative/enriched version was integrated into the in-tree dispatch (`x/tests/tests/libc/c23_cases.c`).

This dual approach was explicitly requested so that later runs can help distinguish "problem in the library" vs. "problem in the compiler".

### 3. C23 Compatibility Suite Integration
The comprehensive external C23 test suite is maintained under
`x/tests/tests/c23/`.
- It provides ~63 feature tests across all categories (core-language, library, time, iec-60559/fromfp+minmax, unicode/char8_t, initialization/structs, stdckdint, stdbit, free_sized, etc.).
- An `xcc-z80` profile + driver/run scripts were added so the suite's matrix runner can be used against this project's toolchain.
- The in-tree `c23_cases.c` was enriched with logic transcribed from the suite, ensuring "all structures" (both C structs like `div_t`/`timespec`/etc. and the full set of test categories) are exercised when running the normal `make -C x/tests/tests/libc core-test`.

### 4. Repo Structure & Distribution Discussion
The project was recognized as becoming complex (toolchain + libc + runtime + full OS + future GUI + tests + packaging all in one tree). The user explicitly wants:
- The ability to build and publish the "x tools" (xcc, xas, xld, ...) as an **independent** distributable.
- Tests that are primarily local to the component that owns the code.
- Still support genuine end-to-end tests that cross components.

A full restructuring proposal was developed (see `x/docs/ARCHITECTURE.md` for the detailed target layout, migration steps, and rationale).

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
    `libcpm3.a`, `libcpc-464.a`, `libcpc-664.a`, and `libcpc-6128.a`
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

- `x/src/` — the X tools (xcc, xas, xld, xopt, ...)
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
- The `x/tests/tests/c23/` suite should remain the authoritative source for the broad C23 matrix; the in-tree dispatch is for fast local verification + libc surface testing.

## How to Resume Work Here

1. Read (in this order):
   - `AGENTS.md` (root)
   - `x/docs/ARCHITECTURE.md`
   - `x/docs/CURRENT-STATUS.md` (this file)
   - Component READMEs (`lib/libc/README.md` or equivalent, `src/yos/README.md`, etc.)

2. The session system will also feed a compacted history when you return to this directory. The documents above are the durable, human-readable memory.

3. Common entry points:
   - `make -C x`
   - `make -C x/tests/tests/libc core-test` (the direct libc + C23 dispatch runner)
   - `bash x/tests/run_tests.sh --filter xcc` for the unified compiler suite

Feel free to ask the AI (or a future human) to re-read these three documents + the relevant source trees at the start of any new session on this project.

## Recent Artifacts Worth Knowing About

- `x/tests/tests/libc/c23_cases.c` — enriched in-tree C23 test (covers all major categories + structs from the external suite + our specific libc additions).
- `x/tests/tests/c23/` — the full C23 compatibility suite + xcc-z80 integration.
- Large test data in `x/tests/tests/libc/test_main.cpp`, `stdio_cases.c`, and `x/tests/tests/runtime/test_*.cpp`.
- All the new C23 assembler implementations live in the existing files under `lib/libc/src/` (math/moremath*.s, stdlib/strtod*.s + heap_core, stdio/printf.s, etc.).

Update this file (and the architecture doc) when significant new work is completed or when the restructuring actually begins.
