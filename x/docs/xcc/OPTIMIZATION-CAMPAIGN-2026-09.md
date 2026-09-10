# September 2026 optimization campaign

This campaign starts from XCC source commit
`1301bcf7a709dfd3cd88031338b57f9ba696cbf9`. The original compiler was rebuilt
from a separate source snapshot before any edits. All seven lanes of the
pinned current z88dk24 comparison were rebuilt and rerun: every status,
linked byte count, and cycle count exactly reproduced the published
`current-results.csv`. This provides a measured baseline rather than relying
on an old executable or mixing different target libraries.

The benchmark corpus, compiler pins, workload constants, expected results,
link options, and emulator timing model are unchanged. All transformations
below use instruction semantics, types, use counts, control flow, memory
effects, or physical register clobbers. There is no recognition of source
names, paths, benchmark identifiers, or workload fingerprints.

## Compiler changes

### Costed modular constant multiplication

A bounded superoptimizer searches coefficients modulo 256 or 65536 with
Dijkstra's algorithm. Its instruction grammar includes doubling, negation,
addition/subtraction of the original operand, byte shifts, and high-byte
addition/subtraction. The state records whether the original operand has
been retained in E/DE, and charges for that copy. `-Os` minimizes bytes then
cycles; `-Of` minimizes cycles then bytes. The size policy also retains the
shared runtime helper when inlining would be larger than the established
call alternative.

The arithmetic body for multiplication by 257 changes from 11 bytes / 107
T-states to 3 bytes / 12 T-states. Multiplication by 65281 changes from 25 /
261 to 3 / 12, and multiplication by 65535 from 32 / 338 to 6 / 24. These are
examples of a search over every coefficient, not a table of selected source
constants. The body measurements exclude operand loading and result storage.

The graph's algebraic and cost invariants were checked for every coefficient
under ASan/UBSan. A release build constructs a 16-bit table in approximately
12 ms and caches it for the compiler process. Optimality is within the stated
instruction grammar; it is not a claim of globally optimal Z80 code.

### Known integer bits and expression chains

Local dataflow records independently known zero and one bits at the actual
integer width, including casts, bitwise operators, shifts, and carry-aware
addition/subtraction. It folds impossible comparisons and redundant masks.
Signed division by a positive power of two becomes a shift only if the value
is nonnegative or its low bits prove exact divisibility; this preserves C's
rounding toward zero. Facts are discarded at control-flow boundaries and
unknown effects rather than guessed across joins.

Adjacent private constant AND/OR/XOR and shift chains combine at their
original width. Truncation, signed extension, full-width shift saturation,
shared values, volatile accesses, and boolean normalization are explicit
constraints. Seven independent expression-chain probes shrink from 165 to
82 code bytes in the final `-Of` / ABI 1 compiler.

### Byte-oriented shift instruction selection

Constant long shifts discard whole byte lanes before emitting the remaining
bit shifts. Word right shifts can keep the low byte in A and use RRA;
near-byte-boundary shifts use direct carry/byte shuffles where their measured
cost beats the existing lowering. Byte shifts can use accumulator rotates
followed by a mask. The cost decisions distinguish `-Os` and `-Of`.

The word lowering was executed over every 16-bit input and every valid count
for left, logical-right, and arithmetic-right shifts in both profiles:
6,291,456 emulator calls, including varying incoming carry flags.

### Typed byte caching across pointer updates

Register-only pointer updates preserve A physically. The backend now retains
an unrelated cached byte through those updates, while invalidating either
cached byte of the pointer being changed. Read-only matching of a byte
shift/XOR transformation also preserves a valid incoming A cache. The cache
rejects volatile and SFR source objects; captured value temporaries remain
eligible. Tests cover both bytes of in-array pointers across all 256 starting
alignments, captured and direct volatile reads, and both ABIs.

This avoids an indexed reload using typed value identities. Assembly reload
and load-narrowing rules still require private-spill proofs before dropping
potentially observable memory reads.

### Constant fills through captured pointers

The loop pass accepts a dynamic pointer only when it is a uniquely defined,
nonvolatile temporary captured before the loop, and retains the existing
checks on pointer/counter uses and loop effects. BLOCK_FILL participates in
normal use/liveness analysis. The backend compares actual costs of direct
stores, partially unrolled DJNZ loops, and overlapping LDIR fills. Speed
mode bounds code growth per fill; size mode chooses the smallest sequence.

A standalone 1,024-byte fill through a pointer has these measured costs:

| Profile | Before bytes | After bytes | Before cycles | After cycles |
|---|---:|---:|---:|---:|
| `-Os` | 63 | 12 | 111,978 | 21,530 |
| `-Of` | 64 | 23 | 111,974 | 14,999 |

Other tested counts include 4, 64, 256, 257, 384, 512, 2,048, and 2,051.
Tests check bounds, empty cases, volatile exclusions, and stack state.

### Proven caller-register preservation

A final assembly pass propagates BC/IY effects through actual instructions,
branches, local calls, shared tails, and recursive call cycles. It runs after
peepholes and outlining, whose scratch-register choices would make an earlier
code-generator-only summary unsafe. Unknown callees, instructions, indirect
control flow, and opaque assembly conservatively clobber both pairs.

Only matching saves marked by XCC as register-only caller saves can be
removed. Argument pushes and stack cleanup remain distinguishable. Removing
an IY save costs four fewer bytes and 29 fewer T-states per call. The pass has
34 focused positive/negative graph tests, including sanitizer checks, plus
C execution tests under both ABIs.

### Profile-specific unconditional jumps

An unconditional JP takes 10 T-states; JR saves one byte but takes 12.
The final peephole keeps JP in `-Of`/`-O3` and performs the size reduction
in `-Os`. Existing JR instructions and conditional-branch policy are
unchanged. This avoids introducing a speed regression during final assembly
cleanup after the higher-level optimizer has made its cost decisions.

## Native runtime and libc

These shared assembly implementations serve every staged X platform and
both optimization profiles. They add no writable static state.

The 32-bit multiply uses
`a0*b0 + ((a1*b0 + a0*b1) << 16)` modulo 2^32. This eliminates unnecessary
absolute values, sign restoration, and a stack-based 32-iteration product.
IX, IY, alternate registers, caller arguments, and stack balance are
preserved. The linked implementation, including newly required helper
bodies, shrinks from 235 to 133 bytes. Across 4,096 deterministic random
products, mean cycles fall from 8,539.64 to 3,881.50 (54.55% fewer).

Memory-copy/fill entry sequences fetch the stack count without an IX frame.
Bounded searches use CPIR/CPDR, and string lengths use the remaining scan
count. Tests cover all low count-byte bit patterns, zero length, byte-count
boundaries, all search bytes, source bounds, return pointers, preserved
registers, and caller/callee stack cleanup.

The table counts each independently linked function plus its required
helpers. Cycle columns use length 256; searches are unsuccessful, and
strnlen exhausts its bound. The same emulator measures both versions.

| Routine | Before bytes | After bytes | Before cycles | After cycles |
|---|---:|---:|---:|---:|
| `memcpy` | 28 | 15 | 5,535 | 5,477 |
| `memset` | 36 | 23 | 5,552 | 5,494 |
| `memchr` | 42 | 22 | 14,742 | 5,489 |
| `memrchr` | 45 | 24 | 12,951 | 5,506 |
| `strlen` | 18 | 18 | 5,497 | 5,482 |
| `strnlen` | 16 | 15 | 15,145 | 5,411 |

## Correctness constraints and repairs

Optimizing a value derived from volatile memory or an SFR does not permit
removing, merging, duplicating, or hoisting the access itself. Exact access
tests complement value tests because reading an unchanged byte twice can
return the expected answer while still violating the program's semantics.
The campaign exposed defects in existing dead-code elimination, scalar
promotion, local expression reuse, loop-invariant motion, delayed source
rematerialization, and register caches. Symbol summaries retain volatility
even when a narrowed use has lost its type qualifier; port operands must
retain their I/O address space through rewrites.

Full-width boolean conversion now normalizes nonzero words, longs, and near
pointers rather than truncating to the low byte. Promoted byte addition and
constant-left subtraction retain the incoming operand before using A as
scratch. Byte-return frame planning accounts for actual temporary stores.
Pointer-fill lowering preserves all effects before the loop, including
stores between counter initialization and loop entry.

The supplemental corpus also exposed stale word-store forwarding: after
`p->next = cursor; cursor = cursor->next`, a later load of `p->next` must
still yield the old cursor. Availability now depends on both the address
and the captured value; redefining either invalidates it, including partial
symbol writes. The shared address resolver expands only unique earlier
definitions in the same block with unchanged inputs. It retains narrowing
and captured-pointer semantics, applies subtraction offsets with the correct
sign, and distinguishes an object's address from its stored value. Generic
scalar, pointer, alias, offset, cast, and memory-sentinel regressions cover
these rules independently of the original linked-list program.

## Final z88dk24 results

The raw [September matrix](../../tests/benchmarks/z88dk24/optimization-2026-09-results.csv)
combines the final two XCC reruns with the unchanged competitor columns from
the complete baseline rerun in this session. All seven original lanes were
rebuilt; their statuses, bytes, and cycles exactly match the previous
[published baseline](../../tests/benchmarks/z88dk24/current-results.csv).
[Executable hashes and provenance](../../tests/benchmarks/z88dk24/optimization-2026-09-versions.txt)
distinguish the locked baseline revision from the measured working tree.

Both XCC profiles pass 24/24 checksums. Against the better valid 80cc
frame-pointer/stack-pointer result per row, `-Os` wins size on 24/24 and
`-Of` wins speed on 24/24. These wins also hold against the valid SDCC
envelope. Including sccz80 and the six expensive SDCC maximum-allocation
probes, `-Of` is now strictly fastest on every row. `-Os` is smallest on
16/24 against that expanded envelope: sccz80 still has eight smaller images.

The prior compiler already won all 24 ordinary speed/size comparisons.
This campaign additionally beats SDCC maximum-allocation on charbench:
26,467,549 versus 26,519,054 cycles. The baseline XCC took 26,800,220.
SDCC's existing bitfield checksum failure remains visible (23/24 correct)
and contributes no performance win. Both 80cc modes and sccz80 pass 24/24;
the expensive SDCC probe passes its six configured cases.

Compared with the rebuilt original compiler:

| Profile | Total bytes before | Total bytes after | Total cycles before | Total cycles after | Cycle reduction |
|---|---:|---:|---:|---:|---:|
| `-Os` | 139,151 | 139,097 | 734,683,642 | 729,793,826 | 0.67% |
| `-Of` | 149,180 | 148,665 | 587,472,837 | 579,599,222 | 1.34% |

The `-Os` total shrinks by 54 bytes (0.039%). MD5 alone shrinks by 447 bytes,
but correct volatile access and conservative assembly memory proofs add code
to many other images. Their cost is included in the totals. `-Of` shrinks by
515 bytes overall, including 1,083 bytes from MD5, and improves cycles on
21/24 rows. Integer arithmetic grows by 42,769 cycles (0.130%), RLE by
19,159 cycles (0.159%), and switchbench by 45 cycles.
These totals sum the configured workloads and do not imply a universal
workload-weighted speedup.

The shared-z88dk benchmark uses the competitors' common CRT/library.
Therefore the native X runtime/libc gains above do not contribute to these
24-program numbers.

| Program | `-Os` byte change | `-Os` cycle reduction | `-Of` byte change | `-Of` cycle reduction |
|---|---:|---:|---:|---:|
| charbench | +17 | 0.153% | +29 | 1.241% |
| crcbench | +17 | 0.082% | +26 | 0.105% |
| intbench | +21 | -0.088% | +28 | -0.130% |
| ptrbench | -21 | 0.762% | +0 | 1.940% |
| md5 | -447 | 0.062% | -1,083 | 1.683% |
| sieve | +21 | -0.002% | +27 | 0.706% |
| rle | +37 | -1.287% | +48 | -0.159% |
| sortbench | +21 | -0.001% | +30 | 0.207% |
| queenbench | +21 | -0.001% | +26 | 0.357% |
| searchbench | +21 | -0.001% | +30 | 0.498% |
| switchbench | +21 | -0.000% | +28 | -0.000% |
| hashbench | +27 | -2.726% | +32 | 0.601% |
| strbench | +17 | 0.819% | +18 | 0.987% |
| histbench | +17 | 14.997% | +23 | 15.154% |
| fixedbench | +17 | 0.022% | +17 | 0.043% |
| bitfieldbench | +15 | 2.447% | +19 | 2.785% |
| vecbench | +21 | -0.001% | +27 | 0.230% |
| matrixbench | +17 | 0.311% | +24 | 0.507% |
| interpbench | +21 | -0.000% | +28 | 0.000% |
| structbench | +21 | -0.004% | +28 | 0.549% |
| recordbench | -5 | 0.001% | +0 | 0.100% |
| listbench | +17 | 0.112% | +26 | 1.633% |
| lexbench | +11 | 1.414% | +24 | 1.186% |
| maskbench | +21 | -0.001% | +30 | 0.477% |

## Validation

Final validation uses the frozen V4 compiler sources and staged S/M/L
prefixes. SHA-256 manifests identify the sources and all four compiler
binaries, including the ordinary M prefix. Earlier exploratory runs and
rejected transformations remain in separate artifact directories; they are
not counted as final passes.

The complete compiler matrix is split into compilation and execution
partitions without reducing its cases or configured optimization profiles.

| Model / ABI selection | Compilation passes | Execution passes | Result |
|---|---:|---:|---|
| S, manifest ABI selection | 2,795 | 1,088 | Passed after timeout retry |
| M, manifest ABI selection | 2,795 | 1,353 | Passed after timeout retry |
| L, ABI 0 | 2,748 | 1,783 | Passed after timeout retries |
| L, ABI 1 | 2,774 | 1,758 | Passed after timeout retries |

All **17,094 distinct compiler variants pass**: S 3,883, M 4,148, L ABI 0
4,531, and L ABI 1 4,532. The L union of compilation and execution IDs
exactly matches the complete canonical selection in each ABI. Required-ABI
exclusions are recorded separately: 57 for ABI 0 and 56 for ABI 1.

The initial parallel run exceeded the 20-second compilation limit for
`t132` at O2 in S, M, and both L ABIs. Exact canonical retries pass under
the unchanged limit. The preserved V3 and final V4 compilers produce
byte-identical assembly and essentially identical CPU time for that case.
The fixed-project `utf8.h` Os case likewise passes its unchanged 120-second
limit on retry. Two L runtime-case compilations, `t123` Os/ieee32 in ABI 1
and `t142` Of/ieee32 in ABI 0, pass their unchanged 30-second limits on
retry. The former also produces byte-identical V3/V4 assembly with essentially
identical CPU time. Original timeout logs and successful retry logs are both
retained; retries do not count as additional unique cases. No manifest timeout,
expected value, or benchmark workload was relaxed.

Additional completed checks:

- 9,568,256 exhaustive emulator executions: every 16-bit input for all
  valid word shifts and 25 constant multipliers, each in `-Os` and `-Of`.
  Every coefficient in both multiplication search spaces also passes
  algebraic/cost checks under ASan/UBSan.
- 738 exact volatile/SFR, repeated volatile-loop, and typed-cache checks
  across S/M/L, optimization profiles, and both ABIs.
- Fresh direct runtime build: 443/443 tests. Fresh libc build: 108/108 core
  shards plus scanning and wide-character suites, including freshly
  compiled C-driven wrappers.
- External C23 matrix: 59 passed, zero failed, four not claimed in each
  of `-Os` and `-Of`. Fixed C23-project corpus: 22/22 in each profile.
  Imported z88dk suite: 280/280.
- Canonical TheAlgorithms/C `-Os` corpus: 51 passed, zero failed, nine
  documented source/target exclusions; the 51 include 11 floating-point
  cases. Supplemental `-Of`: 50 passed, one existing target-capacity failure,
  and the same nine exclusions. The valid bucket-sort program now passes
  with output identical to the host after the forwarding repair.
- X host suites: xz80 56, xld 99, xobjcopy seven, xemu 11 plus stdio,
  xgdb unit/MI/DAP/cpptools, xprog, xar, and compiler ELF symbol metadata.
  Xas passes 1,046 runtime/libc assembly roundtrips and 109 byte-identical
  compiler assembly comparisons, plus its directive, format, and macro
  tests. Its parity script reports 74 existing coverage skips, detailed
  in the host-test artifact README.
- The full xopt smoke suite passes with ASan/UBSan, including 34 caller-save
  graph proofs, 48 spill/access/read-order/value checks, and jump-cost
  assertions for all three optimized profiles.
- Real-ROM/MCP platform checks: all four ZX Spectrum modes and all three
  CPC models pass. Fresh CP/M programs pass hello, input, and 32 argv
  cases. All eight X compilation-chain cases and platform-layout checks
  pass.

Final benchmark correctness is 24/24 z88dk24 programs in both XCC profiles,
20/20 bare programs in each of O2/Of/O3/Os, 40/40 portable programs across
all eight compiler lanes, and 50/50 numeric cases in each of Of/Os. Numeric
return values also agree between the two profiles.

The supplemental `-Of` QR eigenvalue program does not fit the emu memory
layout. Its final 64,908-byte image is unchanged by the late forwarding
repair and extends above the initial stack at `0xfa00` and heap limit at
`0xf000`. The untouched compiler produces a still-larger 65,063-byte image
that fails in the same layout with the same target libraries. The case
passes in `-Os`. Its `-Of` failure is retained explicitly, with baseline
and final maps, hashes, output, and unchanged cycle-limit evidence in the
validation artifacts; it is not relabeled as a pass or silently removed.

The portable suite's 40 generated RLE workloads expose a remaining speed
gap: native SDCC speed wins all 40, with XCC Of taking 27.68% more aggregate
cycles. XCC Os is smaller on all 40, by 15.02% in aggregate. Thus the
z88dk24 speed result is not a claim of universal superiority. The bare
harness's pre-existing native-SDCC failures remain visible: size matches
one result and speed matches none; those invalid results provide no wins.

## Research and competitor audit

The implementation is original. The audit inspected the pinned
[80cc source](https://github.com/z88dk/z88dk/tree/e3aeca0dc09b7b9bbf968b70fc8c29c1f1da208a/src/80cc)
and official SDCC source, particularly constant multiplication,
known-bit metadata, register allocation, and call preservation.

[Souper](https://arxiv.org/abs/1711.04422) motivates deriving and validating
small integer rewrites rather than guessing source idioms. XCC's new
multiplication search is much narrower: finite modular coefficient states
and an explicit Z80 instruction grammar, without an SMT solver dependency.
[LLVM's KnownBits](https://llvm.org/doxygen/structllvm_1_1KnownBits.html)
provides a useful reference for the independent zero/one-bit abstraction;
XCC implements its own conservative typed-IR transfer functions.

## Reproduction artifacts

The ordinary complete matrix commands are:

```sh
make -j12 x-s x-m x-l
make -C x -j4
bash x/tests/run_tests.sh bin/x-s/bin/xcc --filter model-s --work build/check-s
bash x/tests/run_tests.sh bin/x-m/bin/xcc --filter model-m --work build/check-m
bash x/tests/run_tests.sh bin/x-l/bin/xcc --filter model-l --abi 0 --work build/check-l-abi0
bash x/tests/run_tests.sh bin/x-l/bin/xcc --filter model-l --abi 1 --work build/check-l-abi1
```

This session splits compilation and execution work and partitions L execution
by the original listed variant IDs. The artifact ledger verifies that every
eligible ID runs exactly once, with unchanged options, expectations, and
limits, and accounts separately for required-ABI exclusions.

With the pinned current z88dk24 toolchains prepared, rerun the edited XCC
profiles using:

```sh
ALLOW_UNLOCKED=1 XCC="$PWD/bin/x-m/bin/xcc" \
  OUT="$PWD/build/optimization-campaign/z88dk24-repeat" \
  x/tests/benchmarks/z88dk24/run-current.sh --lanes xcc_Os,xcc_Of
```

`ALLOW_UNLOCKED` permits the edited XCC working tree. The recorded competitor
pins, corpus, shared target library, and workload options remain fixed.

Local build artifacts are under `build/optimization-campaign/`:

- `z88dk24-before/`: complete reproduced seven-lane baseline, binaries, maps,
  logs, executable hashes, and raw results.
- `z88dk24-final-v4/`: final frozen-compiler XCC benchmark runs, including
  generated assembly, maps, binaries, checksums, and timing logs.
- `final-validation-v4/`: final model/ABI matrices, external corpora,
  platform tests, direct runtime/libc rebuilds, source/binary SHA-256
  manifests, and machine-readable benchmark status summary.
- `portable-final-v4/`, `bare-final-v4/`, `numeric-Of-v4/`, and
  `numeric-Os-v4/`: complete final benchmark CSVs, summaries, and artifacts.
- `host-tests/`: host-tool logs, exact commands, exit statuses, sanitizer
  results, and the explanation of canonical assembler parity skips.
- `runtime-measurement.txt` and `libc-measurement.txt`: native before/after
  measurements, with the standalone emulator drivers alongside them.
- `dataflow/fill-measurements.txt`: generic fill measurements and generated
  binaries; `dataflow/induction-proof/` checks BC preservation against a
  host-computed checksum.
- Compiler/profile test logs and independently built source snapshots are
  retained beside these measurements.
