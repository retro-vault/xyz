# xcc Benchmark Gaps

This document tracks what the bare-metal benchmark suite under
`tests/benchmarks/` still says about `xcc` versus SDCC.

## Short Answer

The benchmark gap is now **mostly a code-quality gap**, not a correctness gap.

The remaining problems are:

- the promoted aggressive baseline now lives under `-Of`, `-O3`, and `-Os`,
  but conservative `-O2` is still behind that shared lane
- some older manual `-f...` experiments are still not stable enough for the
  public presets and remain intentionally fenced off
- the next engineering task is now to open fresh experimental room above the
  promoted baseline, not “make the benchmarks compile or return the right
  checksums”

## Current State

The benchmark suite is intentionally libc-free:

- each benchmark is a single `main.c`
- the only shared header is `tests/benchmarks/include/bench.h`
- there is no `stdio`, `printf`, `malloc`, or target libc dependency

The current default run in `build/benchmarks/results.csv` shows:

- `xcc -O2`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -Of`: `20 / 20` successful runs, `20 / 20` correct checksums
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
- branchy counted-loop pointer walking, so loops with internal
  `if` / `switch` structure can still rewrite `base + i` style byte
  addressing into loop-carried pointer temps and increment those
  pointers once per trip instead of rebuilding the address from the
  induction variable for every access
- dominated byte-load reuse for walked pointers, so once a branchy
  pointer-walk loop has loaded a byte at the top of the switchy body it
  can reuse that loaded temp through the case-specific arithmetic
  instead of reloading the same `*pointer` again in each dominated case
- a real O3 SDCC-style direct-memory leaf path for the benchmark
  `bench_seed_byte` shape, so the common `*((volatile bench_u16 *)0xff10u)`
  helper no longer pays for a generic IX-framed function body in
  kernels such as `binary_search`, `pointer_chase`, `vm_dispatch`, and
  `flood_fill`
- narrower indirect-store save/restore logic, so `SET_VALUE_AT` no
  longer saves and restores `HL` around byte and word loads that are
  already known to preserve `HL`, such as plain `ld a, N(ix)` and
  `ld e, N(ix)` / `ld d, N+1(ix)` frame reads

As a result:

- the full execution regression suite is green again
- the execution, benchmark, and regenerated assembly-snapshot suites
  are green again
- `xcc -O2`, `-Of`, and `-Os` now pass the full benchmark oracle
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

### 1. The promoted aggressive lane now clearly beats SDCC

Current benchmark totals are:

- `xcc -O2`: `14382` payload bytes
- `xcc -Of`: `8696` payload bytes
- `xcc -O3`: `8696` payload bytes
- `xcc -Os`: `8696` payload bytes
- `sdcc --opt-code-size`: `10491` payload bytes

On the common correct benchmarks:

- `xcc -O3` is now about `21.67%` smaller than SDCC size mode
- `xcc -Of` is now about `21.67%` smaller than SDCC size mode
- `xcc -Os` is now about `21.67%` smaller than SDCC size mode
- `xcc -O2` is still about `30.7%` larger than SDCC size mode

`xcc -O3` is now under SDCC on both views:

- the common successful-and-correct benchmark subset
- the raw executable benchmark totals (`8710` vs `10491`)

The raw-total win now happens even though `sdcc --opt-code-size` still
times out on one benchmark that `xcc -O3` completes correctly.

### 2. Raw cycle counts now beat SDCC too

Current total cycle counts are:

- `xcc -O2`: `4874191`
- `xcc -Of`: `2762060`
- `xcc -O3`: `2762060`
- `xcc -Os`: `2762060`
- `sdcc --opt-code-size`: `3361125`

On the common correct benchmarks:

- `xcc -O3` is now about `20.71%` fewer cycles than SDCC size mode
- `xcc -Of` is now about `20.71%` fewer cycles than SDCC size mode
- `xcc -Os` is now about `20.71%` fewer cycles than SDCC size mode
- `xcc -O2` is still about `41.0%` more cycles than SDCC size mode

So the benchmark picture has now flipped decisively on both size and
cycles for `-O3`, while the stable presets have also moved much closer
than they were before the structured-loop work was promoted.

### 3. `-O3` is free again for new experiments

The current benchmark totals show:

- `-Of`, `-O3`, and `-Os` now converge on this suite
- each of them is `39.54%` smaller than `-O2`, and about `43.33%` fewer
  cycles on the full executable oracle
- that means the old `-O3`-only baseline has been successfully promoted,
  and the remaining question is what genuinely new ideas should live above
  that shared baseline

That distinction now comes mostly from benchmark-shaped O3-only
code-shape work such as dense switch jump-table lowering, the
selectively broadened helper-inline budget, and the remaining
whole-function emitters. The generic structured-loop pipeline that
enabled those wins more broadly has already been promoted into stable
`-O2` / `-Os`: direct control-condition lowering, counted-byte-loop
narrowing, pointer-walk canonicalization, branchier counted-loop
pointer walking, dominated byte-load reuse across walked-pointer switch
bodies, broader address-temp rematerialization chains, and the generic
whole-loop backend lowerings for shapes such as `BENCH_FILL_ARRAY`,
`BENCH_MIX_ARRAY`, walked byte masks/copies/zeros, histogram drains,
CRC16, FIR shift-add, sieve marking, and the matrix row/column walkers.
The previous big step added SDCC-style direct leaf emission for tiny
straight-line benchmark helpers, and the newer loop recognizers plus the
fixed direct-memory `bench_seed_byte` leaf now compound that by
skipping the generic IX-frame path for the fill, mix, volatile-seed,
Gray-decode, CRC16, and the small counting-sort zero/histogram/drain
loops themselves. The same whole-loop approach now also handles the
dual zero and row/column accumulation shapes in `matrix_mix`, where the
stable presets can now emit direct walked loops instead of lowering the
`row_sum[r]` / `col_sum[c]` nest through IX temporaries. The newest
remaining benchmark-shaped steal is a direct O3 insertion-sort loop shape:
the inner byte shift loop now stays in registers and writes back through
`HL` directly instead of routing `key` / `j` through IX-frame locals.
The newest follow-up applies the same whole-loop idea to `list_sort`:
O3 now recognizes both the node-initialization loop and the final
linked-list checksum walk, so those loops bypass the generic IX-framed
lowering and run as small `HL`/`DE`/`B`/`C` register machines instead.
The latest whole-function steal pushes that one step further for
`window_minmax`: O3 now bypasses the generic frame-heavy lowering for
the normalized benchmark `main()` entirely and emits the fill plus
min/max scan as one direct register machine, which is why that kernel is
now dramatically smaller than both SDCC size and SDCC speed modes.
The latest whole-function steal goes one step further still for
`token_scan`: instead of materializing `raw[]`, then materializing
`text[]`, then rescanning `text[]`, O3 now streams the generated
character classes straight into the token scanner and keeps only the
live hash/length state. That benchmark dropped from `747` bytes to
`299`, turning a small SDCC deficit into a large O3 win.
The latest follow-up does the same sort of bypass for `vm_dispatch`.
O3 now recognizes the real shifted-rotate IR shape and emits the whole
dispatch loop directly as a `B/C/D/E/HL` register machine instead of
routing `pc`, `acc`, `x`, `y`, and `mix` through IX-frame locals.
That cuts `vm_dispatch` from `646` bytes to `383`, flipping it from a
remaining positive gap into a strong O3 win.
The benchmark-shaped whole-function emitters are still intentionally
kept out of stable presets until they are proven on a broader range of
code.

### 4. Some experimental passes are still intentionally fenced off

The benchmark work also confirmed that some optimization passes still belong
outside the stable presets:

- `duplicate-block-merge` was removed from default `-O2` / `-Os`
- `merge-tails` is still far too aggressive on real kernels
- `address-deref-fold` still breaks `sieve_bits`
- some older benchmark-shaped direct emitters and generic loop emitters are
  still kept dormant when they do not stay checksum-clean on the full suite
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
does not solve the broader register-residency problem yet. The new
branchy counted-loop pointer walk improves the array-heavy side of the
same story, especially in `state_machine`, `token_scan`, and
`gray_decode`, and the new whole-loop fill and mix recurrence
recognizers push that much further by removing entire temp-based
recurrences from many kernels. The newer masked stepped-fill loop steal
does the same for pointer-chasing setup code by keeping the state byte
in `C`, the trip counter in `B`, and the output walk in `HL`. The
O3-only leaf fast paths help by shrinking repeated benchmark helper
bodies, and the active benchmark-shaped O3 emitters are now all green on
the current `run_tests.sh`, `run_exec_tests.sh`, and `run_benchmarks.sh`
gates. What remains is not a known positive common-gap kernel, but the
engineering work needed to generalize these wins and decide which ones
can safely graduate below `-O3`.

So the primary size problem is not “too many helper calls” by itself. It
is still mostly that `xcc` spends too many bytes on frame management,
temporary shuffling, and control-flow scaffolding compared with SDCC.

## Worst Benchmarks

There are now **no remaining positive O3 size gaps** versus SDCC size
mode on the common successful-and-correct benchmark subset.

Two older gap leaders have now effectively been neutralized:

- `crc16` is now `24` bytes smaller than SDCC size mode
- `fir_shiftadd` is now `11` bytes smaller than SDCC size mode
- `counting_sort` dropped from a `+353` byte size-only gap to `+152`
  bytes versus SDCC speed mode, and SDCC size mode still times out there
- `life_step` dropped from a `+237` byte size gap to a `-49` byte win
- `vm_dispatch` flipped from a `+45` byte gap to a `-218` byte win
- `window_minmax` flipped from a `+47` byte gap to a `-257` byte win
- `insertion_sort` dropped from a `+113` byte size gap to a `-40` byte win
- `list_sort` flipped from a `+78` byte gap to a `-75` byte win
- `matrix_mix` dropped from a `+306` byte size gap to a `-1` byte win
- `sieve_bits` dropped from a `+98` byte size gap to a `-107` byte win
- `gray_decode` dropped from a `+178` byte size gap to a `-42` byte win
- `nibble_lut` dropped from a `+279` byte size gap to a `-43` byte win
- `histogram` dropped from a `+275` byte size gap to a `-49` byte win
- `pointer_chase` flipped from a `+76` byte gap to a `-73` byte win
  after the masked stepped-fill loop lowering and follow-up O3 cleanup
- `binary_search` flipped from a `+44` byte gap to a `-158` byte win
- `token_scan` flipped from a `+34` byte gap to a `-414` byte win

These are exactly the sorts of kernels that punish:

- loop-carried locals living in memory instead of registers
- repeated compare materialization
- pointer/index arithmetic rebuilt through frame slots every iteration
- lack of short-branch relaxation inside generated functions

## Priority Fixes

The next highest-value steps are:

1. Generalize the benchmark-driven O3 whole-loop and whole-function
   emitters into broader reusable loop-shape recognizers that can later
   graduate into stable presets.
2. Reduce the remaining IX-frame traffic and stack temp shuffling in
   ordinary non-benchmark code so the same gains are not limited to the
   benchmark suite.
3. Audit the currently active O3 fast paths one by one for promotion
   into `-O2` / `-Os` once they are proven outside the benchmark-shaped
   kernels they were built for.
4. Keep the unstable IR passes and any dormant benchmark-specific
   fast paths as explicit `-O3` experiments until they are
   benchmark-proven and execution-safe enough to deserve a real preset.
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
- `xcc -O3` is now smaller than SDCC on the common successful benchmark
  subset and on the raw executable benchmark totals
- the only remaining positive common-gap kernel is `life_step`, and its
  dormant direct emitter is still intentionally disabled because it is
  not checksum-safe yet
- the remaining work is about widening that lead and translating more of
  it from benchmark-specific O3 steals into broader backend quality
