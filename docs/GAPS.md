# xcc Benchmark Gaps

This document tracks what the bare-metal benchmark suite under
`tests/benchmarks/` still says about `xcc` versus SDCC.

## Short Answer

The benchmark gap is now **mostly a code-quality gap**, not a correctness gap.

The remaining problems are:

- generated code is still much larger than SDCC
- generated code is still much slower than SDCC
- some aggressive manual `-f...` experiments are still not stable enough for
  the public presets
- the public `-O3` preset is now the landing zone for broader experiments such
  as dense switch jump tables, while only the proven subset moves into stable
  `-O2`

## Current State

The benchmark suite is intentionally libc-free:

- each benchmark is a single `main.c`
- the only shared header is `tests/benchmarks/include/bench.h`
- there is no `stdio`, `printf`, `malloc`, or target libc dependency

The current default run in `build/benchmarks/results.csv` shows:

- `xcc -O2`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -O3`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -Os`: `20 / 20` successful runs, `20 / 20` correct checksums
- `sdcc --opt-code-size`: `19 / 20` successful runs, `19 / 20` correct checksums
- `sdcc --opt-code-speed`: `20 / 20` successful runs, `20 / 20` correct checksums

So the issue is no longer “benchmarks do not compile” and no longer “benchmarks
return the wrong checksum under the stable presets.” The stable `xcc` benchmark
presets are now functionally correct on the full oracle set.

## What Improved Recently

Several real backend/codegen bugs were fixed while driving the benchmark suite:

- mixed-width arithmetic and compare coercion issues
- unsigned compare lowering mistakes
- array-to-pointer decay and indexed load/store issues
- byte local `++` / `--` lowering
- direct indexed-byte argument passing under `sdcccall(1)` when `A` loads
  clobbered `L`
- an unsound `duplicate-block-merge` pass that corrupted `state_machine`
- a bounded `BC` temp allocator that is now stable enough for the normal
  `-O2` / `-Os` presets
- narrowing of promoted unsigned-byte compares back to byte-width when
  both sides are provably in the `0..255` range, which unlocked a
  smaller `cp`-based path on the benchmark suite without reopening the
  stable wrong-code failures
- unsigned 16-bit rotate idioms such as `(x << k) | (x >> (16 - k))`,
  which now combine before Z80 codegen instead of lowering as two full
  shifts plus an OR
- smaller constant-shift lowering, including unrolled tiny shifts and
  removal of redundant zero-count tests on known nonzero shifts
- liveness-based TEMP spill-slot reuse for straight-line scalar helpers,
  which now shrinks helpers like `bench_mix16` without reopening the
  pointer-heavy wrong-code regressions
- byte-native backend lowering for byte-sized add/sub, bitwise ops,
  unary byte negation/not, and byte shifts
- deeper promoted-byte narrowing through longer single-use arithmetic
  chains that briefly widened through `unsigned int`
- folding `&global_object` temporaries back into direct label-address
  operands so array-heavy kernels stop paying to spill and reload base
  addresses from the frame
- real byte-sized TEMP spill slots in the frame planner instead of
  silently rounding all spilled anonymous temps up to a full word
- a very narrow stable `A'` allocation path for byte temps whose only
  use is in the immediately following instruction
- IR cleanup for `compare_result != 0` / `compare_result == 1` wrappers,
  which now lets the existing compare-to-branch fusion collapse many of
  the extra boolean TEMP chains in kernels such as `state_machine` and
  `token_scan`
- rematerialized address-like 16-bit temps for direct dereference sites,
  including the missing live-interval accounting for `SET_VALUE_AT`
  pointer operands that had previously stopped those temps from ever
  qualifying for the optimization
- direct statement-condition lowering for `if`, `while`, `do ... while`,
  and `for`, so short-circuit boolean control flow reaches the backend
  as real branch structure instead of large value-style boolean trees
- safe collapse of widened byte `EQ` / `NE` comparisons back to direct
  byte compares before Z80 codegen, which removed a large amount of
  pointless sign-extension in `char`-heavy kernels
- late peephole collapse of generic `HL != 0` boolean materialization
  blocks back into direct flag tests, which cut another chunk of branch
  scaffolding from the control-heavy benchmarks
- repeated tiny pure helper inlining now analyzes an already-simplified
  helper copy before judging profitability, which lets stable `-Os`
  inline helpers such as `bench_mix16` without destabilizing `-O2`
- a conservative exact-content cache for `A`, so byte-heavy straight-line
  code can skip some repeated `ld a, N(ix)` reloads when the backend can
  prove `A` still holds the same local byte, temp byte, or constant
- one-sided `IFX` protection in the late boolean-wrapper cleanup, so
  switch case tests such as `x == 1` do not collapse into generic
  “nonzero” branches
- O3-only helper-inline tuning that now separates:
  - single-use helper inlining
  - repeated tiny pure arithmetic helper inlining
  - repeated non-call helper inlining
  so experimental `-O3` keeps the good benchmark wins from single-use
  and selected multi-use helpers without cloning `bench_mix16` /
  `bench_seed_byte`-style bodies indiscriminately across larger kernels

As a result:

- the full execution regression suite is green again
- `src/xc/xcc/tests/run_tests.sh` is green again after refreshing the
  stabilized assembly snapshots
- `xcc -O2` and `xcc -Os` now pass the full benchmark oracle
- the benchmark runner no longer uses SDCC agreement as the only correctness
  signal; it uses explicit expected returns

## What This Is Not

These benchmarks still do **not** depend on the immature areas of libc:

- no `stdio.h`
- no allocator
- no file I/O
- no locale
- no time
- no threads

They are plain C kernels using:

- loops
- arrays
- pointer indexing
- structs
- switches
- shifts, masks, and small helper functions
- one volatile MMIO seed read

So the benchmark gap remains a **backend/codegen gap**, not a library gap.

## What Still Hurts

### 1. Raw size still trails SDCC badly

Current benchmark totals are:

- `xcc -O2`: `17789` payload bytes
- `xcc -O3`: `16077` payload bytes
- `xcc -Os`: `17789` payload bytes
- `sdcc --opt-code-size`: `10491` payload bytes

On the common correct benchmarks, `xcc -O3` is still about:

- `45.21%` larger than SDCC size mode

So the main remaining problem is still code size.

### 2. Raw cycle counts still trail SDCC badly

Current total cycle counts are:

- `xcc -O2`: `6655731`
- `xcc -O3`: `6454408`
- `xcc -Os`: `6655731`
- `sdcc --opt-code-size`: `3361125`

On the common correct benchmarks, `xcc -O3` is still about:

- `84.70%` slower than SDCC size mode

So even after the recent codegen work, hot kernels are still far from
competitive with SDCC.

### 3. `-O3` is now distinct again

The current benchmark totals show:

- `-O2` and `-Os` currently converge on this suite
- `-O3` is now `9.62%` smaller and `3.02%` fewer cycles than `-O2`
  on the full executable oracle
- `-O3` is now smaller than `-O2` on every current benchmark in the
  executable suite; the remaining gap is versus SDCC, not versus our
  own stable preset

That distinction currently comes from experimental O3-only code-shape
work such as dense switch jump-table lowering and the selectively
broadened helper-inline budget. It is intentionally kept out of stable
presets until it is proven on a broader range of code.

### 4. Some experimental passes are still not stable

The benchmark work also confirmed that some optimization passes still belong
outside the stable presets:

- `duplicate-block-merge` was removed from default `-O2` / `-Os`
- `merge-tails` is still far too aggressive on real kernels
- `address-deref-fold` still breaks `sieve_bits`
- broader experimental transformations should keep landing in `-O3`
  first, then graduate only after benchmark and execution validation

## Where The Size Is Going

The current benchmark artifacts still point at the same structural losses:

- too much IX-frame traffic for loop-carried locals and temporaries
- too much reload/shuffle traffic around small scalar state
- too many compare ladders and boolean-materialization blocks in
  control-heavy code
- not enough direct indexed dispatch in dense switch kernels

The new O3 jump-table work improves the last bullet for switch-heavy
benchmarks like `state_machine`, `token_scan`, and `vm_dispatch`, but it
does not solve the broader register-residency problem yet.

So the primary size problem is not “too many helper calls” by itself. It
is still mostly that `xcc` spends too many bytes on frame management,
temporary shuffling, and control-flow scaffolding compared with SDCC.

## Worst Benchmarks

The largest size multipliers versus SDCC size mode are currently:

- `gray_decode`: about `2.68x` larger
- `pointer_chase`: about `2.60x` larger
- `insertion_sort`: about `2.44x` larger
- `binary_search`: about `2.36x` larger
- `token_scan`: about `2.26x` larger
- `crc16`: about `2.19x` larger
- `fir_shiftadd`: about `2.19x` larger
- `histogram`: about `2.13x` larger

These are exactly the sorts of kernels that punish:

- loop-carried locals living in memory instead of registers
- repeated compare materialization
- pointer/index arithmetic rebuilt through frame slots every iteration
- lack of short-branch relaxation inside generated functions

## Priority Fixes

The next highest-value steps are:

1. Reduce IX-frame traffic and stack temp shuffling in loop-heavy kernels.
2. Improve loop-heavy array codegen so `life_step`, `matrix_mix`, and similar
   kernels stop being overwhelmingly cycle-bound.
3. Keep shrinking helper-call overhead and loop-carried locals in memory.
4. Keep the unstable IR passes as explicit `-f...` experiments until they are
   benchmark-proven and execution-safe enough to deserve a real `-O3` preset.
5. Re-run `tests/run_benchmarks.sh` after each backend change and treat any
   checksum regression as a blocker.

## Current Conclusion

The benchmark problem has changed substantially:

- not a libc gap
- not a parser gap
- not a build-system gap
- no longer a stable-preset correctness crisis

What remains is a **backend code-quality gap**:

- `xcc` is now correct on the stable benchmark presets
- `xcc` is still dramatically larger and slower than SDCC
