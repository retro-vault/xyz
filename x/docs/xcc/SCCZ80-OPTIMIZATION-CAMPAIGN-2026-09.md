# XCC: closing the sccz80 size gaps

This follow-up implements the generic opportunities from [the sccz80 audit](SCCZ80-SIZE-AUDIT-2026-09.md), then checks the resulting compiler across the regression and benchmark inventories. The previous September campaign remains the comparison baseline for the unchanged pinned toolchains. A separate comparison uses the freshly downloaded September 9 nightly and official SDCC trunk. The original audit explained eight real size losses to sccz80: its generated application and framework code used fewer bytes, even though XCC linked less support code in the decomposed examples. The work below addresses those gaps through general transformations. The final staged compiler is correct on 24/24 programs in both profiles. On the freshly downloaded September 9 toolchains, `-Os` is strictly smallest on all 24 and `-Of` strictly fastest on all 24 against every valid measured competitor. Full regression validation is recorded below.

## Measured results

The [fresh nine-lane matrix](../../tests/benchmarks/z88dk24/LATEST-RESULTS.md)
contains complete linked bytes, T-states and correctness status. Headline 80cc
comparisons use the better valid frame-pointer or stack-pointer result on each
row. The broader envelope includes sccz80, official SDCC and bundled zsdcc,
including their original six-workload maximum-allocation subsets.

| Profile | Correct | Smaller than best 80cc | Faster than best 80cc | Smaller than all valid competitors | Faster than all valid competitors |
|---|---:|---:|---:|---:|---:|
| `-Os` | 24/24 | 24/24 | 11/24 | 24/24 | 11/24 |
| `-Of` | 24/24 | 23/24 | 24/24 | 11/24 | 24/24 |

Both ordinary SDCC implementations fail the retained bitfield correctness case.
Those outputs remain in the raw table and do not count as valid competitors.
All sccz80 and 80cc cases pass. These wins describe this measured corpus, not
all possible programs or all optimization configurations.

The eight size losses identified in the earlier audit are closed:

| Program | Previous XCC `-Os` bytes | Final XCC `-Os` bytes | Fresh sccz80 bytes |
|---|---:|---:|---:|
| sieve | 11522 | 11439 | 11472 |
| sortbench | 5167 | 5044 | 5129 |
| switchbench | 4338 | 4242 | 4318 |
| hashbench | 8118 | 8032 | 8091 |
| histbench | 3810 | 3735 | 3773 |
| fixedbench | 4161 | 4008 | 4154 |
| vecbench | 5110 | 4971 | 5009 |
| matrixbench | 10337 | 10276 | 10317 |

A separate rerun holds the earlier compiler and sysroot pins constant. Every
competitor status, byte count and cycle measurement exactly matches the
previous September baseline. Its [raw follow-up CSV](../../tests/benchmarks/z88dk24/sccz80-followup-2026-09-results.csv)
and [versions](../../tests/benchmarks/z88dk24/sccz80-followup-2026-09-versions.txt)
therefore isolate the net XCC changes:

| Profile | Previous total bytes | Final total bytes | Byte change | Previous total T-states | Final total T-states | Cycle change |
|---|---:|---:|---:|---:|---:|---:|
| `-Os` | 139,097 | 137,428 | −1,669 (−1.20%) | 729,793,826 | 781,627,031 | +7.10% |
| `-Of` | 148,665 | 147,720 | −945 (−0.64%) | 579,599,222 | 575,086,709 | −0.78% |

These are sums of complete workload images and cycle counts, with no empty
program subtraction. All correctness repairs are included. The final precision
repairs cost 24 Os bytes and 1,989,260 T-states in the bitfield workload relative
to the preceding private build; the other 47 XCC cells are unchanged. The Os speed cost
is concentrated in deliberate helper reuse and outlining: histogram falls from
3,810 to 3,735 bytes while rising from 24,372,025 to 64,577,891 T-states. Its Of
counterpart improves from 24,145,665 to 22,777,916 T-states. Of fixed-point and
vector workloads improve by 5.73% and 8.99% respectively in this follow-up.

## Implementation

The size wins come from code generation and representation. The before/after comparison retains its benchmark sources, target libraries, workloads and competitor pins. The new upstream comparison changes the compiler and shared sysroot pins explicitly while retaining the original corpus and workload checks. No compiler rule uses a benchmark name, source fragment, workload constant or program fingerprint.

- Fused widened word products are planned before register homes and stack frames. A generic `(unsigned)((unsigned long)a*b >> 8)` wrapper shrinks from 64 to 6 bytes without changing its multiplication helper. Other valid byte windows receive the same treatment.
- Private straight-line scalar updates receive distinct, single-definition temporaries. Every occurrence must lie in one block and follow a complete definition on every visit. Escaped, observable, partial-view and cross-block values are excluded. A pressure guard retains the existing representation when interleaved reads or externally produced values would disrupt profitable register windows.
- A redundant loop counter can disappear when a live recurrence supplies an equivalent endpoint test. Unique initialization/latch definitions and complete integer representations are required, and the original and replacement sequences, including terminal increments, must not wrap. The allocator can keep the replacement offset in IY while comparing it.
- Short indexed-load sequences can be outlined when total bytes, helper overhead and stack access proofs show a strict saving. Inline IX prologues now establish the same proven frame boundary as shared prologue helpers; stores before allocation remain ineligible. These slower size choices are confined to Os.
- Os avoids retaining cheap scaled indices across calls when their spills cost more than recomputation. Its constant multiplication policy can reuse an already-required helper, charging argument transfers and preservation costs. Of retains its fast inline arithmetic.
- Word values can occupy BC through safe address calculations, with explicit clobber and live-range restrictions. Dying multiplication inputs can hand BC to a result only where the helper and consumer support the transfer. Decision chains retain captured comparison values where their control flow permits it.
- Ordinary indirect word increments use byte memory increments with carry propagation. Adjacent word load/add/store operations retain their address and arithmetic state in registers, including a preserved IY cursor. Volatile, atomic, partial, aliased-value and live-result forms retain their required accesses and values.
- Constant global member addresses avoid unnecessary address temporaries. Immutable strings share compatible suffixes, including terminators and embedded-zero payloads; writable arrays retain their own storage. Literal typing, UTF-8 byte storage, pointer initialization and wide-array initialization were repaired alongside the pooling work.
- Constant standard memset calls use existing block-fill lowering when their typed contract and cost permit it. All arguments are evaluated once, including zero-count volatile arguments. Returned-pointer and dynamic-fill cases retain calls where inlining would grow an Os call site. `-fno-memory-builtins` disables the substitution; `memset_explicit` is excluded. The classic runtime's reversed helper contract is recognized only under its explicit runtime profile.
- Ordinary local aggregates initialize their zero representation with the existing costed block-fill operation, followed by the original explicit initializer stores. Recursive volatile/atomic checks retain observable initialization. This avoids reconstructing and spilling one address per byte when the frame grows beyond indexed displacement range. Both size and speed profiles benefit.
- The native 16-bit multiply helper uses shift carry directly and returns immediately for an already-zero result. It shrinks from 48 to 45 bytes and runs faster. Both runtime source copies are synchronized; this does not alter the pinned classic library used for the cross-compiler comparison.

Changes that improve both objectives are enabled in Os and Of; O3 retains its Of alias. Deliberate size-for-speed tradeoffs remain in Os.

## Heap comparison

The supplied Alto calloc uses checked shift/add multiplication instead of multiplying modulo 65536 and dividing back to detect overflow. The adapted native libc routine also clears storage with a seed byte and LDIR, preserves IX/IY, and keeps the requested count across malloc's permitted clobbers. It retains XCC's zero-size allocation policy. The routine shrinks from 120 to 67 bytes. With an identical malloc stub, calloc(1,1024) falls from 45,623 to 21,736 T-states. The five changed allocator modules save 24 bytes after adding overflow and invalid-arena guards; unneeded multiply/divide dependencies can save another 92 linked bytes. [The heap comparison](HEAP-COMPARISON-2026-09.md) records the exact measurements and contracts.

The complete Alto allocator is a different tradeoff: a single address-ordered free list uses a two-byte allocated-block header, reuses free payload bytes for links, and coalesces at the insertion position. XCC keeps an eight-byte block header with an owning-heap pointer, multiple-heap support, and aligned-allocation wrappers. Alto's smaller metadata is real, but dropping these contracts would not be an equivalent replacement. Its supplied code also does not round odd requests to the alignment that XCC intends to provide. Default-heap setup now aligns an odd platform base, and malloc/realloc reject size-rounding overflow without damaging an existing allocation. Explicit custom arenas retain their exact-span API and caller-supplied alignment; the existing custom-heap fixtures are unchanged. Alto's malloc/free/init envelope is 264 bytes against XCC's 567 bytes. It also wins the recorded allocation/free trace, so the whole-allocator comparison remains an opportunity rather than a claimed XCC victory.

## Fresh upstream comparison

The official archive `z88dk-20260909-895dc13665-25851.tgz` was downloaded afresh and checked against a separate recursive upstream Git clone at `895dc1366573b792d98b0607fb1048d2db248b3d`. The archive SHA-256 is `7f314bb2fc843c34ccfbe84e82911eb1b9f4a4d5a1e8ed0f3ca69b12abc04455`. The nightly's 80cc sources include the September 8 change `e56f2c22b321e55a852c3c159d9cbfdaa3533bac`; this is newer than the previously used 80cc branch. sccz80 is built from the same archive.

The nightly still bundles patched zsdcc revision 16639. A separate fresh official SDCC trunk build uses revision 16858, commit `a1ab477f6e5117e74964e7088db566521f7aed7a`. Both are visible in the nine-lane matrix, including the original six-workload expensive allocator probes. Compatibility patches are retained explicitly: XCC per-link option forwarding, the existing z88dk ABI adapter for official SDCC, and an alias for the renamed upstream peephole predicate so the nightly's rules are actually applied. No competitor optimization algorithm is changed. Every lane uses the same fresh headers, CRT and classic target library. Invalid cells remain visible and do not count as winning competitors.

The compiler inventory and nightly build route follow the [official z88dk documentation](https://github.com/z88dk/z88dk/blob/master/README.md). Exact source revisions, binary hashes, archive hashes, commands and compatibility patches accompany the results.

## Additional measured workloads

These native-runtime suites remain independent of the z88dk shared-library
comparison. The unmodified XCC bare, portable and numeric workloads all pass.
The figures below compare with the previous September campaign, including
correctness-repair costs.

| Suite/profile | Cases | Total byte change | Total cycle change |
|---|---:|---:|---:|
| Bare `-Of` | 20 | −18 (−0.15%) | −0.32% |
| Bare `-Os` | 20 | −436 (−3.78%) | +4.51% |
| Portable `-Of` | 40 | −720 (−3.77%) | −7.07% |
| Portable `-Os` | 40 | −720 (−3.79%) | +1.31% |
| Numeric `-Of` | 50 | unchanged | unchanged |
| Numeric `-Os` | 50 | −1,299 (−1.51%) | +1.20% |

The bare suite also passes `-O2` and `-O3`, and the portable suite passes
`-O3`; its output matches Of. Native SDCC remains faster in aggregate on the
portable corpus: 1,581,657 T-states against XCC Of's 1,876,756. XCC uses
18,356 total bytes against 22,554. The z88dk24 sweep therefore does not imply
that XCC wins every independent workload.

The Algorithms QR eigenvalue program now fits the emulator memory map and
produces the exact host stdout in both profiles. Its Of image shrinks from
64,821 to 52,410 bytes (−12,411), and Os from 56,389 to 44,434 (−11,955).
The measured final runs take 200,382,641 and 205,115,048 T-states respectively.
The isolated aggregate patch produced 52,387/44,411-byte images; the final
images include another 23 bytes of heap correctness support. The reduction
comes from ordinary automatic aggregate initialization: code
for the large test objects previously reconstructed and spilled each byte's
address. The algorithm body, data and test expectations are unchanged; the isolated
size attribution holds the runtime constant, while the final images include
the allocator correctness guards.
The size attribution and byte-identical map reconstruction are retained in
`build/sccz80-campaign/final-validation/qr-size-audit/`.

## Correctness work

The investigation also found defects that passing benchmark checksums did not cover. Regressions now exercise one-time volatile switch evaluation and call arguments, single evaluation of complex read/modify/write addresses, byte and word access counts, captured values across branches and loop backedges, partial local writes, transitive register lifetimes through nested loops, typed literal arrays, conversion precision, global stores from function arguments, complete wide-integer truth tests, and preservation of wide return values through assembly optimization.

Cast and comparison source substitution now requires unique definitions, dominance and preservation of the captured source on every relevant path. A source mutation after one loop use remains relevant when control reaches that use again without recapturing it. A 100,000-graph independent state oracle checked the path proof, in addition to compiled execution tests.

The loop and value-versioning work follows established compiler techniques. [LLVM's induction-variable pass documentation](https://llvm.org/docs/Passes.html#indvars-canonicalize-induction-variables) describes recurrence and exit-test simplification. [Braun et al.'s SSA construction paper](https://compilers.cs.uni-saarland.de/papers/bbhlmz13cc.pdf) explains the single-definition representation; this change uses a deliberately bounded one-block form and does not introduce a whole-function phi representation.

Return-value register metadata now survives shared assembly tails. This prevents an assembly peephole from deleting the high half of a wide ABI return merely because it appears dead at a local return instruction. Unknown public entry points remain conservative. The model covers both ordinary and alternate Z80 register banks.

Conditional expressions now convert the selected branch to the common result type, after semantic analysis resolves call return types. A 16-bit negative branch of a 32-bit conditional previously supplied an invalid high word. Tests cover signed and unsigned widths, volatile objects, selected calls, pointer and aggregate results, and void branches. The full-width truth tests that exposed the error are retained.

Further independent oracles exposed storage-width assumptions outside the benchmark corpus. Product-slice selection now requires complete 16/32/16-bit representations before replacing casts with register moves, including the older late-selection paths. Known-bit facts use declared precision and its real sign bit. Byte widening retains a conversion when negative values must wrap at a partial unsigned word's width. Combining shifts gives the synthesized count an ordinary integer type so it cannot wrap in a narrow count operand.

Post-increment load projection requires a complete eight-bit result and preserves full volatile/atomic reads. Deferring a cursor write also requires a private, unescaped ordinary cursor, since a character read can alias the pointer object's representation. Byte-extension facts require the consumer and defining temporary to have the same numeric view: signedness, precision, storage width and byte offset. These repairs preserve ordinary full-width selection and are covered by independent value and exact-access oracles.

Typed byte shift/XOR diamonds retain their carry-based implementation through integer join conversions that preserve all eight low bits. Boolean conversions and partial-byte precisions remain ineligible. The negative execution oracle also exposed IR passes that removed a partial-byte narrowing cast after relabelling arithmetic with its storage size; their destination proof now requires a complete eight-bit representation.

The stricter type-compatibility checks exposed qualifier loss in inherited typeof/typedef types and in memchr's generic macro. The fixes preserve inherited qualifiers and select the search return type through an unevaluated common-void-pointer expression. The C23 corpus also contained one incorrect expectation: a wide string literal has unqualified wchar_t elements, so wcschr applied to it returns wchar_t*. That assertion is corrected and paired with a separate const-array assertion, following [N3096 sections 6.4.5 and 7.31.4.6](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf). The raw earlier failure remains recorded.

C23 bit-precise integer promotions and usual arithmetic conversions now use the declared precision rather than the rounded storage size. Bit-precise integers are exempt from ordinary integer promotion; standard integer types win equal-width rank ties. The independent host type oracle checks both operand orders, signedness combinations and plain-char modes. These rules follow [the C23 working draft, sections 6.3.1.1 and 6.3.1.8](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf).

Dynamic byte shifts now restore the value before the zero-count exit. An independent emulator oracle executes all byte values and all valid counts for left, logical-right and arithmetic-right shifts, in both ABIs and every profile.

The host build now compiles the hot assembly-optimizer library with `-O2` by default, while respecting an explicit `CXXFLAGS` override. A scoped cache also avoids rescanning the complete public-symbol declaration list for every return-liveness query. In a reproduced large UTF-8 translation, host optimization reduces compile time from more than two minutes to about 43 seconds, with identical emitted assembly. Tests compare the cached query with the original scan across nested invocations, mutation and threads.

## Validation and limits

The final combined validation covers 18,019 selected compiler regression
variants: 17,955 canonical S/M and L ABI0/ABI1 variants, plus 64 cases discovered
by an independent audit of missing model tags. Exact selection and executed-ID
checks found no missing, extra or duplicate variants. All are covered by passing
results on the final staged compiler.

The original canonical run recorded 17,951 passes and four launch failures in
the new core307 test. Its manifest incorrectly put the command and arguments in
one field and used unsupported placeholder names. Only that launch metadata was
corrected; all four exact in-tree variants then passed through the actual runner
with the same compiler and unchanged semantic payloads. Each performed 1,480,752
precision/access checks and 8,192 positive projection checks. The original four
failures and all 17,967 original evidence files are preserved. The resolved
coverage therefore uses four explicit reruns, not rewritten raw statuses.

| Final validation group | Result |
|---|---|
| Canonical compiler variants | 17,955 covered: 17,951 original passes + 4 manifest-only reruns |
| Independently discovered untagged variants | 64/64 passed |
| XCC benchmark cells | 444/444 passed: fresh/current/historical z88dk24, bare, portable and numeric |
| Imported external corpora | 544/544 applicable checks passed |
| Direct runtime | 444/444 passed |
| Direct libc | 108 named groups plus scanning/wide executables passed |
| Host tools | All 10 phases passed |
| Exact-access and platforms | S/M/L probes, all four ZX48 modes, all three CPC models, CP/M and full-chain checks passed |

The external total comprises 280 imported z88dk tests, 51 Algorithms cases in
each profile, 22 fixed C23 projects in each profile and 59 C23 matrix cases in
each profile. The existing nine Algorithms skips and four unclaimed C23 cases
per profile remain explicit; they are not passes. Both QR executions match host
stdout exactly, and the large UTF-8 project passes its unchanged time limit.

Before the manifest repair, hashes verified all 1,694 production files, 5,678
test payloads and 548 staged files unchanged throughout the combined run. The
post-repair audit permits exactly that one manifest delta and verifies the
other 5,677 payloads, every production/staged file, runner and inventories
unchanged. There was no compiler rebuild between the main run and the four
reruns. Commands, complete group counts, source/tool hashes, original statuses
and repair evidence are in the
[machine-readable validation record](SCCZ80-VALIDATION-2026-09.json). Raw logs
remain under `build/sccz80-campaign/final-validation-v6/`. Isolated improvements
are not added together to estimate final results.

The Os helper-reuse and outlining policies can increase execution time substantially in a hot loop. Histogram is a measured example; Of keeps the faster policy. The supplemental portable RLE suite still provides an independent speed challenge against native SDCC. The earlier Of QR eigenvalue capacity failure is resolved by the measured aggregate-initialization improvement above. Preexisting non-ASCII wide Unicode literal payload limitations were reproduced before and after this work and were not changed by suffix pooling.

An independent alignment audit also reproduced an existing global-object alignment defect in the baseline and revised compiler: declarations can report an alignment that the emitted data and linker contribution layout do not enforce. Correcting that requires coordinated compiler, assembler/object and linker metadata. The heap changes preserve raw custom-arena layout and align the default allocation base; they do not claim to repair global-object placement.

The advertised `_BitInt` front-end range and `BITINT_MAXWIDTH` currently extend
to 64, but the preexisting backend storage layout caps these objects at four
bytes. Dynamic widths above 32 therefore remain an existing unsupported and
misadvertised surface. Independent 33-bit signed/unsigned narrowing reproductions
fail in the prior and revised O0 compiler; they are retained under
`build/sccz80-campaign/final-validation-v5/ir-precision-audit/`. The new dynamic
precision oracles cover the implemented range through 32 bits. Host type and
constant-expression checks through 64 bits do not establish backend support.
