# How sccz80 wins eight size comparisons

The subsequent [implementation and validation report](SCCZ80-OPTIMIZATION-CAMPAIGN-2026-09.md) closes all eight size gaps and includes a freshly downloaded September 9 compiler comparison. This document preserves the original audit and its measurements.

This source and binary audit follows the
[September optimization campaign](OPTIMIZATION-CAMPAIGN-2026-09.md). It does
not change either compiler, the benchmark corpus, compiler flags, or the
published results. The optimization opportunities below are findings, not
implemented compiler improvements.

## Measurements and attribution

The sccz80 executable is the real compiler from the pinned z88dk checkout
`fce9a75f105337c5bcd4e054838a3636d2e1a851`. The current runner gives both
compilers the same target sysroot and benchmark/framework sources. The XCC
compatibility shim contains only a symbol alias and adds no instructions.

These are complete linked binary sizes, including the framework and
zero-filled BSS, from the
[final matrix](../../tests/benchmarks/z88dk24/optimization-2026-09-results.csv).
All eight rows pass the benchmark's correctness checks in both lanes.

| Program | sccz80 bytes | XCC `-Os` bytes | XCC excess |
|---|---:|---:|---:|
| sieve | 11,472 | 11,522 | 50 |
| sortbench | 5,129 | 5,167 | 38 |
| switchbench | 4,318 | 4,338 | 20 |
| hashbench | 8,091 | 8,118 | 27 |
| histbench | 3,773 | 3,810 | 37 |
| fixedbench | 4,154 | 4,161 | 7 |
| vecbench | 5,009 | 5,110 | 101 |
| matrixbench | 10,317 | 10,337 | 20 |

The losses range from 0.17% to 2.02% of the sccz80 image. XCC `-Os` is
faster in six of these eight rows; sccz80 is faster in switchbench and
slightly faster in fixedbench. XCC `-Of` remains fastest on all 24 rows.

Two images were decomposed using symbol addresses, module boundaries,
section sizes, and disassembly:

| Component | Histogram sccz80 / XCC | Vector sccz80 / XCC |
|---|---:|---:|
| Benchmark machine code | 337 / 410 | 403 / 511 |
| Framework machine code | 539 / 602 | 539 / 602 |
| Benchmark/framework strings | 234 / 235 | 239 / 240 |
| Other CRT/library/helper code and data | 2,184 / 2,084 | 2,277 / 2,206 |
| BSS included in image | 479 / 479 | 1,551 / 1,551 |
| **Total bytes** | **3,773 / 3,810** | **5,009 / 5,110** |

XCC links less support code in these examples. sccz80 wins because its
generated application and framework code is smaller. Both use printf mask
`0x201`, identical BSS, and the same 584 bytes of precompiled C library
functions. One misleading comparison must be avoided: sccz80 puts literals
in `rodata_compiler`, whereas XCC includes them in `code_compiler`. Raw
`code_compiler` section sizes therefore do not measure the same thing.

The saved sccz80 histogram and vector binaries were rerun with
`z88dk-ticks -w 60 -b msx`. Both reported one passed test and no failures,
and reproduced exactly 86,484,812 and 25,810,894 T-states. XCC `-Os` takes
24,372,025 and 15,649,663 respectively.

## 1. Shared operations instead of long inline arithmetic

sccz80's constant multiplication selector in
`build/toolchains/z88dk-current/src/sccz80/codegen.c:2104–2198` uses a
small collection of short instruction sequences, then falls back to a
constant load and a call to `l_mult`: six bytes at the call site. The
constant-right-shift selector at lines 3656–3707 similarly calls a shared
shift helper for many counts beyond its short inline cases.

In the histogram example, sccz80's `hist_pass` is 82 bytes and XCC's is
96. A multiplication inside it occupies an 18-byte inline chain in XCC;
sccz80 uses the six-byte load/call sequence. This explains a real size
tradeoff, but does not imply a twelve-byte whole-program saving from
changing one call: helper bodies and register preservation also cost bytes.
The sccz80 multiply's cold dependency closure costs 74 bytes, so this
particular arithmetic decision alone is larger on first use. XCC already
links its unsigned multiplication core and ABI adapter in the histogram
image; another use would add no helper bytes, but still needs correct
argument transfers and preservation of live BC. The 18-byte inline chain
takes 142 T-states. The shared shift alternative costs six bytes per site
and ten bytes for its helper.

The XCC source exposes the policy responsible:
[z80gen_arith.cpp](../../src/xcc/src/backend/z80/z80gen_arith.cpp), around
lines 1914–1924, retains the previous `op_count <= 20` size-mode admission
rule alongside the new minimum-byte multiplication search. The search
finds the best inline sequence within its grammar; that does not establish
that inlining beats a shared call.

A generic improvement is to compare total emitted bytes for inline and
call alternatives, including argument movement, caller saves, helper
dependencies, and reuse. Repeated expressions can justify a helper even
when its first use cannot. XCC already has assembly outlining; a stronger
cost model must account for those later savings too. This policy belongs in
`-Os` when the smaller alternative is slower. The existing short, fast
superoptimized arithmetic remains useful in both profiles.

## 2. Compact stack helpers and fewer temporary spills

sccz80 commonly omits an IX frame and uses shared stack-load helpers. For
example, `l_gint2sp` has a nine-byte body and a three-byte call site. Its
return-address-adjusted stack load is defined in
`build/toolchains/z88dk-current/libsrc/l/sccz80/9-common/l_gint2sp.asm`.
Related helpers cover other offsets and combined load/push operations.
The optional speed-inlining rules in `lib/z80rules.8` explicitly trade
more bytes for fewer cycles.

The shared framework's executable bytes decompose as follows. Outlined
bodies are charged to their owning functions.

| Function | sccz80 | XCC `-Os` | Difference |
|---|---:|---:|---:|
| Assert_real | 51 | 65 | +14 |
| suite_run | 387 | 415 | +28 |
| suite_setup | 31 | 29 | -2 |
| suite_add_test_real | 49 | 67 | +18 |
| suite_add_fixture | 21 | 26 | +5 |
| **Total** | **539** | **602** | **+63** |

The three-byte helper calls are smaller at each use than XCC's two indexed
byte loads, which occupy six bytes, but their body and execution overhead
must be amortized. A nine-byte helper needs four uses to beat the six-byte
inline load on total size; three uses tie. The helper call takes 72 T-states
versus 38 for the two indexed loads, excluding frame setup. In this
framework, thirteen calls across four helpers cost 75 bytes against 78
bytes for the indexed loads. Avoided frame setup and teardown contribute
additional savings.

The framework also shows XCC maintaining both an index and its doubled
value in stack slots. The second recurrence costs seven initialization
bytes and fourteen latch bytes while saving one byte per indexed use.
Recomputing a cheap scale can be smaller than retaining another spilled
induction variable. The existing avoidance guard for cheap scaled
recurrences across calls is disabled specifically for `-Os` in
`x/src/xcc/src/opt/iropt.cpp`, around line 16572. Its profitability should
be reconsidered with actual stack costs.

A more substantial allocation problem appears in fixedbench's multiply
and shift wrapper: sccz80 emits 30 bytes, XCC emits 59. XCC already selects
the narrow unsigned 16-by-16-to-32 multiply and fuses the subsequent
shift/truncation. Nevertheless, it moves incoming register arguments through
three pairs of stack slots in a twelve-byte frame before calling the helper.
The operation was recognized, but the resulting register transfers and
frame were not reduced accordingly. This points to general cast/copy
coalescing and frame planning after arithmetic fusion, with opportunities
to improve both profiles.

An independent source reproduces the problem without benchmark context:

```c
unsigned product_window(unsigned left, unsigned right) {
    unsigned long product = (unsigned long)left * (unsigned long)right;
    return (unsigned)(product >> 8);
}
```

Its standalone wrapper is 27 bytes in sccz80 and 64 bytes in XCC, excluding
helper bodies. The XCC frame entry is five bytes larger here because it
is inline rather than shared. `z80gen_arith.cpp:1641–1646` explains why
the emitter uses materialized widened temporaries: their original source
registers may already have been reused. Register-home and frame planning
precede that emission in `z80gen.cpp:1215–1236`. Eliminating the spills
requires earlier fusion or planning that understands fused operands;
simply bypassing the loads in the existing emitter would be unsafe.

## 3. Memory updates can beat load/compute/store

sccz80's `lib/z80rules.1:2304–2318` recognizes an indirect word increment
and uses byte memory increments with carry propagation. XCC's histogram
code instead contains this ordinary-memory sequence:

```asm
ld e,(hl)
inc hl
ld d,(hl)
inc de
ld (hl),d
dec hl
ld (hl),e
inc hl
```

It occupies eight bytes and takes 52 T-states. When the incremented value
in DE and the flags are dead, a smaller form is:

```asm
inc (hl)
inc hl
jr nz,done
inc (hl)
done:
```

This occupies five bytes and takes 29 T-states normally, or 35 when the
low byte wraps. An independent xz80 harness executed both sequences for
all 65,536 input words, both incoming carry states, and ordinary and
page-crossing addresses: **524,288 executions passed**. Both leave HL at
the high-byte address and preserve surrounding memory, SP, A, BC, IX, IY,
and alternate registers. Mean new cost is 29.0234 T-states.

This is a prospective compiler rule, not an installed optimization. It
requires proof of ordinary nonvolatile memory, dead DE result and flags,
and no stronger access/atomicity requirement. In particular, it omits the
high-byte read and write when carry is absent, so it must not be used for
volatile or device memory. It is a candidate for both `-Os` and `-Of`.

## 4. sccz80 does not win every individual lowering

The main switch dispatch in switchbench occupies 48 bytes in XCC.
sccz80 uses 56 bytes at the site, plus a shared linear-search dispatcher.
Its twenty-byte whole-image win comes from other code, not a better
dispatch-table algorithm. The complete accounting is 105 extra bytes of
XCC benchmark/framework code and literals, offset by 85 fewer support
bytes.

sccz80 also searches within its literal pool, allowing a string to reuse
a suffix of a previous string. See `src/sccz80/const.c:380–408`.
XCC's `merge_duplicate_string_literals` in
[irgen.cpp](../../src/xcc/src/ir/irgen.cpp) merges exact duplicates. Suffix
pooling accounts for the framework's one-byte difference: sccz80 reuses
the first string's terminator for the empty string, while XCC allocates a
separate byte. It is another generic size opportunity, requiring careful handling
of encoded widths, embedded zero bytes, alignment, and relocation offsets.

## Correctness findings encountered during the audit

Two independent XCC reproducers were obtained while examining the
framework. Passing benchmark checksums do not exercise these obligations.

```c
volatile unsigned control;
unsigned choose(void) {
    switch (control) {
    case 0: return 41;
    case 1: return 42;
    case 2: return 43;
    default: return 44;
    }
}

unsigned increment(unsigned input) {
    volatile unsigned local = input;
    ++local;
    return input != 0;
}
```

The switch lowering uses the volatile object separately in each case
comparison instead of capturing the controlling expression once. The IR
and assembly show this in `-O0`, `-O1`, `-O2`, `-O3`, `-Of`, and `-Os`,
under both ABIs. The front-end source is
[irgen_stmt.cpp](../../src/xcc/src/ir/irgen_stmt.cpp), around lines 270–283.
It is independent of the new optimization campaign's changes.
An instrumented device returning successive values 1, 2, and 3 makes the
generated code return the default result 44 instead of the required
one-time sample's result 42. With a fixed value of 2, each byte is read
three times.

The increment example triggers `rule_superopt_ix_word_inc_direct` in
[z80peep.cpp](../../lib/xopt/src/z80peep.cpp), around lines 15005–15145.
Its conditional high-byte increment lacks an eligibility check excluding
volatile source slots. Instrumented execution with input 1 confirms that
`-Os` omits the high-byte read and update performed at `-O0`; input 255
touches both bytes because the low byte wraps. sccz80 also conditionally
updates the high byte of the framework's volatile counter, so this is not
evidence of better sccz80 semantics. These are unresolved findings from
this audit; the compiler was not modified to work around them.

## Retained evidence

Paths below are relative to the repository root:

- `build/optimization-campaign/z88dk24-before/work/`: original sccz80
  binaries, linker maps, sources, and successful execution logs.
- `build/optimization-campaign/z88dk24-final-v4/work/`: corresponding
  final XCC binaries and maps.
- `build/optimization-campaign/sccz80-audit/framework/`: annotated
  framework disassemblies, independent volatile reproducers, and emitted
  IR/assembly for all twelve profile/ABI combinations. Instrumented
  execution results are in `access_probe-results.txt`.
- `build/optimization-campaign/sccz80-audit-switch/`: switch and fixed-point
  disassembly used to identify the dispatch and temporary-spill costs;
  `product_window.c` and both compiler outputs preserve the independent
  multiply/shift reproduction.
- `build/optimization-campaign/sccz80-word-inc-audit.cpp` and `.log`:
  independent exhaustive execution harness and its result.

The next general optimization targets are better inline-versus-helper
costing, less stack traffic after fused operations, profitable memory
updates, and suffix pooling. Correctness repairs to volatile handling must
remain separate from those size choices.
