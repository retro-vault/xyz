# Audited XCC Baseline And Graduated Speed Profiles

This report supersedes and retracts the earlier tuned-corpus leaderboard.
That result was not evidence of general compiler performance: an audit found
complete-function structural recipes developed while repeatedly measuring
this corpus.  They did not inspect filenames, but recognizing the entire IR
shape of a benchmark-derived function is still benchmark recognition.

The corpus is pinned at
`460ec34769f01324ca49b66145f09babdbe507fc`; the competitor toolchain under
`orig/z88dk` is pinned at
`94bc327720721402eee7ffc1c4794245fe65ebb1`.

The overfitting-audited baseline had:

- no whole-function `try_emit_sdcc_style_helper` selector
- no fixed-global interprocedural benchmark specialization
- no public-preset use of the then-unsafe scalar-local promotion, physical
  register allocator, or promoted-byte-operations experiments
- identical `-O3` and `-Of` optimization settings at that audit point
- the same supported public ABI behavior under `sdcccall(0)` and
  `sdcccall(1)`

## Current Graduated Result (2026-08-06)

Guarded scalar promotion, physical register homes, the dense-dispatch
profitability guard, and local adjacent-recurrence spill sinking have
graduated to `-Of`. The recurrence rewrite also belongs to `-Os` because it is
strictly smaller as well as faster. `-O3` is an empty experimental alias of
`-Of` and is omitted from this report to avoid duplicate rows. These
transformations match IR shape, types, liveness, aliases, and control flow
only; they do not inspect source paths, function names, benchmark constants,
or expected outputs. All 23 programs pass in both ABIs.

| XCC lane | Correct | Size wins vs SDCC | Speed wins vs SDCC | Size strict best | Speed strict best |
|---|---:|---:|---:|---:|---:|
| `-Os`, ABI 1 | 23/23 | 23/23 | 3/23 | 22/23 | 1/23 |
| `-Os`, ABI 0 | 23/23 | 23/23 | 3/23 | 22/23 | 1/23 |
| `-Of`, ABI 1 | 23/23 | 22/23 | 14/23 | 22/23 | 13/23 |
| `-Of`, ABI 0 | 23/23 | 22/23 | 14/23 | 22/23 | 13/23 |

The Pareto-safe recurrence graduation gives `-Os` a third speed win against
SDCC and its first strict speed win against the full competitor envelope,
without weakening its 23/23 size wins against SDCC.

By geometric mean, ABI1 `-Of` is 0.65% faster than SDCC across all 23
programs. It is still 9.89% slower than the per-program best successful
competitor envelope (ABI0: 9.72%), so the result is a large improvement rather
than a claim that every remaining code-generation gap is closed.

The current raw CSV, binaries, maps, outputs, and run logs are generated under
`build/x/benchmarks/benchmark23-current/z88dk24/`. The historical audited
measurement below is retained to show the earlier baseline and the reason for
the guardrails. The compiler used for this promotion has SHA-256
`ee69e040f25df2068ca0a9a27c1be839ab4c04edca0780a2e0a3c282c5a000cd`.

## Measurement

Every lane compiles the same source and framework, runs the complete linked
image, and must produce the expected result.  Every XCC, SCCZ80, SDCC, and
80CC image now executes in the same repository `z80_exec` Z80 model.
The z88dk trap adapter handles its test CRT's console and file calls without
altering target instruction counts.

Sizes are complete linked-image byte counts.  They are useful product
measurements, but include each compiler's CRT and formatting support and
therefore do not isolate C code generation.

**XCC's size in this report is not on the same baseline as SCCZ80/SDCC/80CC
here, nor as any of the four in upstream z88dk's own published benchmark
table.** SCCZ80/SDCC/80CC link against z88dk's own CRT and C library in both
places (the shared baseline that makes their sizes comparable to each
other). XCC in *this* report links against its own minimal `--platform=emu`
runtime instead, which is far leaner and not size-comparable to the other
three columns.

Upstream z88dk's `zcc` has a `-compiler=xcc` integration (`-S --sdcccall 0
--c1mode`, linking XCC's assembly output against z88dk's own CRT/library —
the same baseline as the other three) that puts XCC's size on equal footing.
**As of 2026-08-07 this now works end to end** and reproduces upstream's
published XCC numbers almost exactly (23-benchmark cross-check: all 46
`-Os`/`-Of` lanes built, ran correctly, and matched upstream within ~1% size
/ a few % cycles — see below). It needs six things, none of which live in
this repository (all are local-only patches to a separate z88dk checkout,
since they change *z88dk's* source, not XCC's):

1. z88dk's `src/zcc/zcc.c` only defines `-D__XCC` for the xcc compiler
   branch, not `-D__SDCC`. z88dk's own headers (`include/sys/proto.h`'s
   `__ZPROTO*` macros and others) branch on `__SCCZ80` / `__SDCC` / a
   generic fallback that assumes GNU `__attribute__((overloadable))`
   support and declares library calls under a *different*, double-underscore,
   reversed-argument name (`__fread`, matching a newlib/ez80-clang-style
   ABI) that the classic `test_clib` library used by `+test` does not
   provide. XCC does not take that fallback path correctly and needs the
   `__SDCC`-branch declarations instead (single-underscore, normal argument
   order) — which is also the *correct* choice, since XCC already implements
   SDCC's own calling conventions (`--sdcccall 0`/`1`). Fix: also pass
   `-D__SDCC` alongside `-D__XCC` in that one `zcc.c` branch.
2. Two of XCC's own runtime helpers are referenced by its generated code but
   only exist in XCC's own runtime archive, never in z88dk's library, because
   this path never links that archive: `___printf_sd` (XCC's constant-format
   compact-printf codegen shortcut for `printf` calls using only `%s`/`%d`/
   `%i`/`%%`) and `__sdcc_call_hl`/`__sdcc_call_bc`/`__sdcc_call_iy` (indirect
   call trampolines for calls through a register-held function pointer).
   Fix: link one extra tiny shim object providing these — `___printf_sd` as
   a tail-jump to z88dk's own `_printf` (identical sdcccall(0) stack-argument
   layout, so this is a faithful bridge, not a behavior change), and the
   three call-trampolines as their literal XCC-runtime bodies (`jp (hl)`,
   `push bc \ ret`, `push iy \ pop hl \ jp (hl)` — see
   `x/src/xcc/lib/runtime/jumps/*.s`, all self-contained, no further
   dependencies).
3. z88dk's headers use a `__preserves_regs(regs...)` variadic
   function-attribute macro (`include/sys/compiler.h`) that's defined as a
   no-op only in the CLion/IntelliSense IDE-tooling branch, not in the real
   compile path — real compilers are expected to understand it as a native
   attribute keyword. XCC's parser doesn't. Fix: pass
   `-D'__preserves_regs(...)='` on the command line to blank it out (only
   needed by benchmarks whose headers transitively pull in the declarations
   using it, e.g. `sieve`, `rle`, `sortbench`, `md5`).
4. z88dk's own sdcc-dialect translator (`lib/sdcc/sdcc_opt.1`) maps a raw
   `.area _DATA` — the area XCC emits for every initialized global, labels
   *and* initializer bytes together — to `SECTION bss_compiler`, a group
   z88dk-z80asm treats as compiler-generated BSS: it reserves the address
   range but discards the bytes, because real SDCC pairs `.area _DATA` with
   a separate `_INITIALIZER`/`_GSINIT` copy-up sequence that actually
   supplies the values at startup. XCC never emits that GSINIT sequence (its
   own linker places `_DATA` pre-populated in the flat image, no copy-up
   needed), so under z88dk's CRT every nonzero-initialized global — and even
   `static const` data, since XCC also routes that through `_DATA` — silently
   reads back as zero. Fix: append one more rule to the local `xcc_rules.1`
   copt-rules file (`z88dk-native-xcc-bench` reference memory has the exact
   text) that runs *after* z88dk's own sdcc-dialect pass and rewrites
   `SECTION bss_compiler` to `SECTION code_compiler` — the group `_CODE`
   already maps to, whose content z88dk-z80asm does preserve verbatim.
5. z88dk's `include/sys/proto.h` prototype macros (`__ZPROTO3` etc.) append
   a bare `__smallc` keyword to classic-library declarations like
   `open()`/`read()` to mark their true Small-C left-to-right argument order
   (confirmed by reading `open.asm`/`read.asm` and the `z88dk-ticks` host
   trap handler directly: `open`'s registers are `HL`=filename, `DE`=flags,
   `BC`=mode — the reverse of what a right-to-left push would produce).
   XCC's parser already understands this natively as `[[z88dk::smallc]]` —
   but only if it reaches the token. z88dk's own `compiler.h` blanks
   `__smallc`/`__z88dk_callee`/`__z88dk_fastcall` to nothing whenever `__XCC`
   is defined (a leftover "make IntelliSense easier" branch, originally
   meant only for `__clang__`/tooling passes), and the `-D__XCC` this path
   already defines (for the fix above `-D__SDCC` needed) trips that branch,
   so z88dk's own external preprocessor (`z88dk-ucpp`, run *before* XCC ever
   sees the source) erases the keyword before XCC's parser — which does
   recognize it — gets a chance to. Fix: in the local z88dk checkout's
   `include/sys/compiler.h`, split that block so `__XCC` gets its own arm
   mapping the three keywords to XCC's modern attribute spelling
   (`[[z88dk::smallc]]` etc.) instead of blanking them.

6. XCC's own runtime helper for a 16×16→32 unsigned multiply-widen,
   `___muluint2ulong` (`x/src/xcc/lib/runtime/int32/muluint2slong.s`), takes
   its two arguments in `HL`/`DE` (registers) — matching XCC's own internal
   convention for this helper. z88dk's SDCC-compatible library ships a
   *same-named* symbol (`libsrc/l/sdcc/___muluint2ulong.asm`) that instead
   expects both arguments pushed on the stack, matching genuine SDCC's ABI
   for it. Under XCC's own linker this is fine (caller and XCC's own
   implementation agree); under z88dk's linker the caller still emits the
   register-based call sequence, but resolves against z88dk's stack-based
   implementation, so the multiply runs on stack garbage instead of the real
   operands. Symptom: `fixedbench` (a pure-integer Q8.8 fixed-point DSP
   benchmark — it does not use `double`, unlike an earlier draft of this
   section claimed) computed a wrong checksum at every optimization level.
   Fix: add `___muluint2ulong`'s body (self-contained: `IY`, a 16-iteration
   shift-add loop, no further dependencies) to the shim object from patch 2,
   so it resolves before z88dk's conflicting archive member is ever pulled
   in — same pattern as `___printf_sd`/`__sdcc_call_*`.

With all six applied, `zcc +test -compiler=xcc -Cx-Os` (or `-Cx-Of`) plus
the shim object builds and correctly runs all 23 upstream programs (see table
below) — including `fixedbench`, previously and incorrectly documented here
as needing `double` support the M-model doesn't provide; it needs neither
`double` nor the L-model, just patch 6 above. Sizes/cycles match upstream's
own published table almost exactly — typically ±0.5% size and low
single-digit % cycles, well within normal measurement/version noise.

Patches 4–6 above fixed the three programs that originally surfaced genuine
open problems in this exact configuration. `switchbench` (wrong
VM/switch-dispatch result) and `interpbench` (hangs, runs out its cycle
budget) were both symptoms of patch 4's DATA-init bug: `switchbench`'s
bytecode program table and `interpbench`'s equivalent were silently reading
back as zero, so both were dispatching on garbage/zero opcodes. `md5`
computed the wrong hash for a different reason at each optimization level:
at `-Os` its file-read path (`open()`/`read()`) was fetching a return value
from the wrong register, fixed by the real in-repo XCC bugfix described
below; at `-Of`/`-O2`/`-O3` it was a *second*, unrelated bug (see the copt
paragraph a few lines down) that happened to reproduce only at those levels.
`fixedbench` failed identically at every level (patch 6, above).

A **genuine, in-repo XCC compiler bug** was found and fixed while chasing
`md5`'s `-Os` failure, independent of the z88dk-side patches above. XCC's
`Z88DK_SMALLC`/`Z88DK_FASTCALL` calling conventions only redefine argument
order (that's their whole purpose — matching Small-C/fastcall's non-standard
push order for real z88dk library interop), but XCC's backend was also
routing their *return value* through the "modern" (`sdcccall(1)`-style) `DE`
register family instead of the plain `HL` every hand-written z88dk
classic-library function actually returns through (confirmed directly via
`z88dk-ticks -trace`: `read()`'s result sits in `HL` right after its `ret`,
matching `cc_z88dk_fastcall`'s own already-correct callee-side codegen,
which contradicted the caller-side assumption). A caller compiled with one of
these ABIs would fetch a stale leftover register instead of the real return
value. Fixed by moving `Z88DK_SMALLC`/`Z88DK_FASTCALL` from the `DE` family
to the `HL` family in `word_return_family()` (and its duplicate in
`z80gen_regalloc.cpp`), `cc_z88dk_smallc`'s `emit_return_value`/
`emit_store_call_result` (`z80gen_convention.cpp`), and the two direct-return
`ex de,hl` fast paths in `z80gen_ctrl.cpp`/`z80gen_arith.cpp`. There was no
prior exec-level test coverage of `[[z88dk::smallc]]`/`[[z88dk::fastcall]]`
return values (existing `core` tests only check that they compile), so this
had never surfaced; a new one should be added. Full re-validation after the
fix: runtime tests 442/442, the complete `xemutest` suite 4291/4291, and this
report's own `--platform=emu` corpus 23/23 — no regressions.

`md5`'s separate `-Of`/`-O2`/`-O3` failure (a genuinely wrong hash, not the
empty-input hash the `-Os` bug produced) turned out **not** to be an XCC
bug at all, and not the optimizer-masking pattern first suspected either.
Isolated to a single-block repro (`MD5("abc")`, reproducible standalone with
no file I/O), the actual computation was correct — what was wrong was patch
4's own copt rule silently failing to fire for `PADDING[]`, a second
`static`-initialized array in the same file, at these optimization levels
specifically. Root cause: `copt`'s lexer breaks on literal double-quote
characters inside a `%title` comment line (they collide with its own
`%"..."N`-quoted-class pattern syntax), which the rule's original verbose
title had; separately, `copt` also silently failed to match that rule when
it was positioned *after* both the `enter_ix` and `leave_ix` rules in the
same file, regardless of title content — an ordering sensitivity, not
understood beyond "it reproduces and reordering fixes it." Neither of these
is whitespace-related; an earlier hypothesis (XCC emits `.area _DATA` with a
literal space instead of a tab at `-Of`, so the rule's literal-tab pattern
silently missed it) looked promising and even *matched the observed symptom*
one investigation-turn earlier, but turned out to be a wrong diagnosis discovered
only by testing the "fix" and watching it still fail — the real cause was
found by isolating the rule into `z88dk-copt` directly and bisecting title
text and rule order by hand. Fixed by shortening the rule's title (detail
moved here instead) and moving it earlier in `xcc_rules.1`, before
`enter_ix`/`leave_ix`.

None of the six z88dk-side patches above are committed anywhere in this
repository: they are patches to a *separate*, external z88dk checkout used
only to validate against upstream's own benchmark methodology, not something
this project's build depends on or ships. The `Z88DK_SMALLC`/
`Z88DK_FASTCALL` return-register fix, by contrast, *is* a real in-repo XCC
change (see the file list above), since it's a genuine compiler
correctness bug independent of any z88dk-side quirk. This project's own
harness (`x/tests/benchmarks/z88dk24/run.sh`) still uses XCC's
`--platform=emu` runtime, by design, since it doesn't require patching a
third-party toolchain and its cycle counts remain fully valid (see below) —
only its XCC size numbers are on a different baseline than the other three
columns. The z88dk-native harness now builds XCC's L-model (`bin/x/bin/xcc`)
rather than the M-model used earlier in this investigation; this ended up
being unrelated to the actual `fixedbench` bug (patch 6) but there is no
reason to prefer the narrower M-model here, so the switch is kept.

**`sccz80` is no longer included in the competitor comparison below** — its
results (see the historical CSVs still under `bench_reconcile/` if needed)
were consistently far enough behind SDCC and 80CC on this corpus that they
added noise without adding a meaningful comparison point.

Cycle counts do not have the baseline problem at all: CRT/library choice
only affects one-time startup overhead, negligible against the measured
workload, so XCC's cycle counts in *this* report (via `--platform=emu`) are
directly comparable to SCCZ80/SDCC/80CC's and to upstream z88dk's own
`z88dk-ticks -w 30 -b msx` measurements, with or without the size-baseline
fix above. Confirmed 2026-08-07 by rebuilding upstream z88dk from source and
cross-checking: with a fresh `z80_exec` build, SCCZ80 matched upstream's
`z88dk-ticks` byte-for-byte and cycle-for-cycle on all 23 programs, and
XCC's `-Os`/`-Of` cycle counts (`--platform=emu`, i.e. this report's own
methodology) were within a few percent of upstream's published table
(typically under 2%, matching normal measurement noise) — i.e. no
methodology drift in the speed numbers, only in the size baseline described
above (and now separately confirmed via the native `-compiler=xcc` path
too).

Run the final matrix with:

```sh
OUT=build/x/benchmarks/z88dk24-final-honest \
  bash x/tests/benchmarks/z88dk24/run.sh
```

Raw results, binaries, maps, and logs are under
`build/x/benchmarks/z88dk24-final-honest/`.

## Honest 23-Program Result

All four XCC lanes execute correctly on 23/23 programs.  The two 80CC lanes
execute 22/23 correctly; their failing `lexbench` results are excluded when
forming that program's best-successful-competitor envelope.

| XCC lane | Correct | Size strict best | Size within 5% | Speed strict best | Speed within 5% |
|---|---:|---:|---:|---:|---:|
| `-Os`, ABI 1 | 23/23 | 22/23 | 23/23 | 0/23 | 1/23 |
| `-Os`, ABI 0 | 23/23 | 22/23 | 23/23 | 0/23 | 1/23 |
| `-Of`, ABI 1 | 23/23 | 22/23 | 22/23 | 1/23 | 2/23 |
| `-Of`, ABI 0 | 23/23 | 22/23 | 22/23 | 1/23 | 2/23 |

Geometric-mean ratios against the best successful competitor for each
program are:

| XCC lane | Linked size | Executed cycles |
|---|---:|---:|
| `-Os`, ABI 1 | 38.07% smaller | 68.24% slower |
| `-Os`, ABI 0 | 39.22% smaller | 68.39% slower |
| `-Of`, ABI 1 | 34.26% smaller | 46.40% slower |
| `-Of`, ABI 0 | 34.91% smaller | 46.18% slower |

Against upstream SDCC specifically, `-Of` wins speed on 5/23 programs.
Against the full successful competitor envelope, it wins only
`lexbench`.  There is no honest 5–10% general speed lead.

`-Os` is not faster than the speed profile.  Relative to `-Os`, `-Of` uses
12.98% fewer cycles under ABI 1 and 13.19% fewer under ABI 0 by geometric
mean, at a 6.14% and 7.09% respective size cost.

## Size-to-Speed Promotion

Only three size-path assembly transforms were promoted to the speed path,
because their Z80 costs are also Pareto improvements:

- remove a repeated register-pair immediate load across stores when the pair
  is proven unchanged
- allocate two- or four-byte stack regions with one or two `push af`
  instructions (11/22 cycles rather than 27)
- compact an otherwise unused temporary frame with the same four-byte speed
  threshold

The five-byte and larger push sequence is slower and remains rejected in
speed mode.  Tail merging, repeated-sequence outlining, and other
call-introducing size transformations remain `-Os`-only.

## Independent Holdouts

The generated portable corpus runs every compiler through the same runner.
All eight compiler/profile lanes return the expected result on 40/40
programs.  On this frozen RLE-family corpus:

- XCC `-Os` is 15.75% larger and uses 73.48% more cycles than the best
  competing size lane in aggregate, with 0/40 size wins
- XCC `-Of` and its empty `-O3` alias are 20.57% larger and use 69.22% more cycles than the fastest
  competing lane in aggregate, with 0/40 speed wins
- XCC `-Of` and `-O3` are identical at 27,194 payload bytes and 2,676,535
  cycles; all 40 outputs are correct

The separate, more varied 20-program bare-metal corpus returns the expected
result in 20/20 programs for each of XCC `-O2`, `-Of`, `-O3`, and `-Os`.
There `-Of` and `-O3` are identical at 13,813 payload bytes and 4,617,452
cycles across the complete corpus.
Most SDCC images in that harness time out or return a different checksum, so
that corpus is used as an XCC generalization/correctness holdout, not as a
cross-compiler leaderboard.

The numeric holdout passes 50/50 XCC executions: ten programs across
fixed8.8, fixed16.16, fixed24.8, float, and double.

## Validation

The preceding full-project frozen validation used compiler hash
`4a6024d831357b05b238a0920f0cf8c49bf114016b5a76d6c650843025fc8c77`
for both the normal and M-model staged compiler.  It completed:

- ABI 0 compile matrix: 2684 passed, 0 failed, 16 skipped
- ABI 1 compile matrix: 2688 passed, 0 failed, 12 skipped
- ABI 0 execution matrix: 1477 passed, 0 failed, 10 skipped
- ABI 1 execution matrix: 1452 passed, 0 failed, 35 skipped
- upstream z88dk compatibility suite: 280 passed, 0 failed
- official C23 matrix: 59 passed, 0 failed, 4 features not claimed
- external C projects: 22 passed, 0 failed
- algorithm corpus: 101 passed across `-O0` and `-Os`, with 19 documented
  source/target-constraint skips
- runtime: 441 passed, 0 failed
- XZ80: 56 passed, 0 failed
- XLD: 95 passed, 0 failed
- XAS local/library suites: 1021 passed, 0 failed
- XAS parity/link comparison: 93 passed, 0 failed, 62 cases skipped where
  both linkers reject unresolved symbols
- XObjCopy: 7 passed, 0 failed
- full-chain integration: 8 passed, 0 failed

Libc partitions, XOpt, symbol metadata, XGDB protocols, XEmu, XAR, CP/M,
YOS application images, and all MDR smoke/stress/reproducibility/size phases
also completed successfully.

Twenty-seven stale exact-assembly assertions tied to removed experimental
recipes were retired.  Their manifests remain, and executable coverage for
the corresponding operations and both ABIs remains in the matrices above.
The 73 matrix skips are explicit manifest ABI constraints filtered out from
the opposite-ABI invocation, not runtime failures or timeouts.
