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
