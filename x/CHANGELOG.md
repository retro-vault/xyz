# Changelog

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release.

## Unreleased

- Fixed three XCC miscompilations exposed by Sah Partner. Compound `+=` and
  `-=` on pointers now scale integer operands by the pointed-to type, explicit
  function-designator dereferences such as `(*compar)(a, b)` call the pointer
  instead of loading a word from the function's code, and reused incoming
  register arguments are spilled before address selection can overwrite their
  ABI register. The last defect made the chess attack generator mistake the
  `king_dirs` table address for a square number and increment bytes in its own
  `generate_moves` machine code. Executable regressions cover all optimization
  profiles.
- Fixed five correctness holes in the optimized Z80 register-home pipeline.
  Lazy incoming-argument spills now use the base word slot even when the first
  consumer is a high-byte view, and conditional first uses conservatively
  preserve values needed on another control-flow arm. Modern-ABI stack
  parameters selected for IY are initialized from their IX slot without
  destroying an earlier HL argument; direct memory copies recognize both
  base and constant-displaced IY pointers for loads and stores. Finally,
  scalar-local promotion no longer maps multiply-defined `_Bool` loop latches
  to a non-SSA temporary. Execution regressions cover every case at `-O0`,
  `-Os`, and `-Of`.
- Corrected Z80 physical register allocation to classify `ADDRESS_OF` by its
  pointer value width rather than the size of the referenced aggregate, so
  taking the address of a large array no longer disables allocation for an
  entire function. Added conservative DE homes for independent word loop
  recurrences, BC/IY homes for dual byte cursors with early exits, and
  shift-add-byte fusion for both explicit-cast and value-propagated IR forms.
  These source-independent changes reduce shared-z88dk benchmark geometric
  mean cycles by 10.1% at `-Os` and 10.7% at `-Of`; all 23 programs pass in
  both profiles. Long byte data directives are also split into 16-value lines
  so downstream copt/z80asm processing cannot reject a generated source line.
- Fixed four XCC floating-point lowering defects exposed by an analog-clock
  application: calls now materialize all required argument conversions
  (including `float`/`double` to `int`), unary expressions refresh their type
  after a call's return type is resolved, and negative/arithmetic floating
  constant expressions are emitted correctly in static initializers. The
  unoptimized integer-to-`float` cast path now uses the direct `*int2fs`
  helpers instead of requiring an unavailable double runtime in the M model.
  Matching execution regressions cover both `float` and `double`. Also fixed the
  CP/M 3 `gettimeofday` backend to preserve its destination pointer across
  BDOS and 32-bit conversion helper calls instead of writing the result to
  page zero, and to forward the actual 32-bit multiplier past its wrapper
  frame rather than multiplying by bytes from the return address.
- Restored SDCC-compatible `sdcccall(1)` stack cleanup for non-variadic void
  and up-to-16-bit-return functions. XCC had incorrectly made every spilled
  argument caller-clean, so calls into callee-clean assembly SDK routines such
  as Lunatik's `gputglyph` and `gputtext` advanced `SP` twice, corrupting the
  intro screen and eventually crashing. Added caller, callee, size-mode, and
  external-void-call regressions while retaining caller cleanup for wide
  returns as required by SDCC's return-sensitive ABI. Updated the hand-written
  `memcpy`, `memset`, `memccpy`, `strlcat`, `strlcpy`, and `strncat` entry
  points to remove their spilled delimiter/count/size words as that ABI
  requires; this also keeps optimized SP-relative local addresses correct
  after any of these calls.
- Graduated the guarded 2026-08 `-O3` speed experiment. Physical register
  homes, scalar-local promotion, the dense-dispatch profitability guard, and
  adjacent 32-bit recurrence spill sinking now belong to `-Of`; the recurrence
  rewrite also belongs to `-Os` because it is strictly smaller as well as
  faster. `-O3` once again has no exclusive transformation and is an exact
  experimental alias of `-Of`, ready for the next speed investigation. The
  23-program report omits the duplicate `-O3` rows while the profiles are
  identical, and every historical O3-only compiler manifest now also executes
  its graduated `-Of` lane. On the full-program matrix the
  recurrence graduation gives `-Os` a third speed win against SDCC and its
  first strict speed win against the complete competitor envelope without
  weakening its 23/23 size wins.
- Made `-O3` a materially faster experimental profile by enabling the
  source-independent physical-home allocator together with unaliased scalar
  promotion. A structural dense `EQ`/`IFX` dispatch guard retains the stable
  scalar form when promotion lengthens a switch selector's live range. The Z80
  backend now sinks stores across adjacent, fully proven 32-bit conditional
  right-shift/XOR diamonds, keeping recurrence state in `DEHL` and committing
  it once at the final join. Intervening IR, different objects, volatility,
  escaping labels, and live branch temporaries remain barriers. Also fixed
  modern-ABI materialization of a byte argument received in `L`: a direct
  `L`-to-frame store no longer destroys an independently live first argument
  in `A`. Independent recurrence and register-home execution tests cover the
  new paths. Broad holdout validation also tightened two general legality
  proofs: loop pointer walking no longer follows a copied induction value when
  another definition can reach it through a nested-loop backedge, and xopt's
  IX self-store cleanup is restricted to compiler-described temporary-frame
  slots and preserves the preceding load whenever `A` remains live. The
  latter now requires an all-path overwrite proof before deleting both
  instructions; source-local stores remain intact when volatility metadata is
  unavailable. Independent nested-subscript and accumulator-liveness
  regressions cover both failures. The 23-program full-image comparison remains correct in both ABI
  modes; `-O3` now has 14/23 speed wins against SDCC and 13/23 strict speed
  wins against the complete non-XCC competitor envelope, versus 5/23 and 1/23
  respectively for the unchanged `-Of` lane. Its geometric-mean cycles are
  0.65% below SDCC and 9.89% above the best-successful-competitor envelope.
- Split `-O3` from the validated `-Of` baseline so it can serve as the
  experimental speed lane while byte-count-only policy remains exclusive to
  `-Os`. Proof-bounded direct truncated-byte lowering and masked-MSB
  shift/XOR lowering now also run under `-Os`, capturing their size wins
  without enabling speed-for-size tradeoffs. Benchmark 23 now executes and
  reports explicit `-O3` lanes under both ABIs, retaining independent
  binaries, maps, correctness, size, and cycles for all six XCC lanes.
- Fixed XCC static pointer initializers so object, array-element, structure-member,
  and function addresses are emitted as relocatable symbols (including byte
  addends) instead of zero-filled data. External symbols referenced only by
  static initializer data are now declared to the assembler as well.
- Added source-independent, proof-bounded Z80 improvements to the validated
  `-Of`/`-O3` pipeline without reintroducing whole-function benchmark
  selectors. Immediately truncated add/sub/bitwise/negate/complement/shift
  expressions now stay byte-wide when their low-byte inputs and adjacent
  cast chain prove that equivalent; adjacent volatile reads remain in place.
  Distinct one-use widening copies are bypassed for shifts so carry-based
  accumulator fusions remain visible, while other byte copies are retained
  when they shorten allocation live ranges. Unsigned-byte right shifts now
  take the same path only when the pre-promotion source proves zero extension;
  signed bytes retain arithmetic word shifting. The branchless byte identity
  `(x << 1) ^ ((0 - (x >> 7)) & p)` is canonicalized by typed dataflow into a
  carry-controlled shift/XOR diamond, with one-definition/use, volatility,
  address-escape, and input-liveness guards. Constant `SRL A` chains may pass
  flag-transparent spills before rotate/mask extraction only when complete
  flag deadness is proven.
  Late assembly cleanup removes canonical IX frames after either frame
  compaction proves zero storage or an exact compiler-described temporary
  allocation is dead. Nonzero allocation removal additionally proves the
  emitted body has no IX, SP, stack, allocation-result HL, or allocation-flag
  dependency. In compiler-identified C functions,
  return-dead `add a,#1` uses `inc a`, while carry-consuming paths and
  unannotated assembly retain `add`. The existing constant block-fill
  recognizer now accepts non-escaping word counters that remain in stack
  slots, removes uniquely dead address setup, recompacts the local frame, and
  emits a frameless `LDIR` fill. Selection uses operation width, adjacency,
  definition/use counts, qualifiers, natural-loop structure, lifetime, and
  actual register/stack references--never benchmark identity, source paths,
  or symbol names. Across the frozen 116-source non-benchmark integer
  development set, every changed translation unit improves: ABI0 saves 414
  bytes and 2,261 static cycles, while ABI1 saves 421 bytes and 2,305 static
  cycles.
  On the untouched 20-program bare-metal holdout, all XCC lanes still pass
  with correct checksums; `-Of`/`-O3` totals improve from 18,963 to 17,391
  payload bytes and from 7,237,623 to 6,578,283 measured cycles. The untouched
  40-program portable holdout remains 40/40 and improves from 28,794 to
  26,834 payload bytes and from 2,974,856 to 2,827,190 cycles; all 50 numeric
  lanes pass. All four XCC lanes also remain 23/23 on the full-program suite;
  the default-ABI `-Of` aggregate saves 362 linked bytes and 7,977,133 cycles.
- Completed the final XCC optimization audit and retracted the earlier
  tuned-corpus leaderboard. The exact whole-function
  `try_emit_sdcc_style_helper` selector and pending fixed-global
  interprocedural specialization were removed because complete-function IR
  recognition is benchmark overfitting even without filename checks.
  Scalar-local promotion, physical register allocation, and promoted-byte
  operations were moved outside every public preset after frozen holdouts
  exposed miscompilations; at that audit point `-O3` became an exact alias of
  the stable `-Of` pipeline. The newer guarded `-O3` experiment is recorded
  above.
  Twenty-seven stale exact-assembly assertions tied to the removed recipes
  were retired while their manifests and executable coverage were retained.
  The frozen ABI matrices execute 8301 variants with zero failures
  (ABI0: 2684 compile + 1477 run; ABI1: 2688 compile + 1452 run), plus 73
  manifest-declared opposite-ABI skips.
  Every compiler lane in the 23-program comparison now uses the same
  `z80_exec` timing model. All four XCC lanes pass 23/23. `-Os` is strictly
  smaller than the best successful competitor on 22/23 programs, but `-Of`
  is strictly faster on only 1/23; its geometric-mean cycle count is 46.40%
  above that envelope under ABI1 and 46.18% above it under ABI0. The earlier
  speed-lead claim was therefore an artifact, not a general result.
  The independent portable corpus passes 40/40 in every lane, but XCC wins
  neither size nor speed there; the separate bare-metal holdout passes 20/20
  for every XCC profile, and the numeric holdout passes 50/50.
  Promoted only Pareto-safe size transforms to the speed path: redundant
  pair-immediate reload removal, two-/four-byte `push af` stack allocation,
  and unused temporary-frame compaction through four bytes. Slower
  call-introducing and five-byte-or-larger size transforms remain `-Os`-only.
  Also fixed an ABI1 compact-frame comparison defect where a reserved spill
  slot for an incoming register parameter was treated as initialized stack
  storage and could leave a frameless function's SP unbalanced.
- Historical tuning of both `sdcccall(1)` and `sdcccall(0)` on the canonical
  23-program z88dk full-image corpus added several generally useful changes,
  but the associated perfect-sweep performance claim is retracted by the
  overfitting audit above. Immutable compare operands and conditional byte
  addresses can remain in IY, while nonzero and coupled loop values can remain
  in BC. The former tuned build's reported `sieve` speed record is not a
  result of the audited compiler. ABI0 additionally promotes only private, direct,
  default-ABI helpers to the register ABI inside a translation unit; public,
  indirect, variadic, inline-assembly, and explicitly attributed boundaries
  retain their declared ABI.  Recursive calls are recollected after parameter
  remapping, and BC cursor reservations now cover their complete live ranges.
  Dead address producers are no longer regenerated inside fused 32-bit
  expressions, and large `-Os` modules retain scalable tail merging and
  repeated-sequence outlining beyond the full peephole optimizer's line
  budget.  Unbanked all-zero globals now occupy startup-cleared `_BSS` rather
  than physical image bytes.  Constant `printf` formats limited to `%s`,
  `%d`, `%i`, and `%%` select a compact general formatter; dynamic or richer
  formats retain the complete implementation. All four XCC lanes passed 23/23
  programs, but the original leaderboard counts included the now-quarantined
  whole-function selector and must be treated as tuned-corpus results. Capped
  byte scans now keep independent byte values in D/E across wide BC bounds
  and preserve them only around indexed global-array address formation; this
  reduces RLE from 11.64M to 9.26M cycles.  Inlining and frame-pointer
  omission remain enabled.  The old reduced z88dk suite was removed in favor
  of this corpus, whose reports retain all 184 emulator images and derive each
  byte count directly from that binary.  The canonical Model-M suite passes
  3696/3696 under ABI1 and 3686/3686 under ABI0.
- Added pinned, executable upstream z88dk torture coverage for the micro-Max
  chess engine and Henry Spencer regexp implementation.  Deterministic
  harnesses exercise the complete recursive chess search and varied regexp
  compile/match, capture, repetition, character-class, and error paths across
  the optimizer profiles; both the normal and M-model toolchains pass all 9
  configured variants.  The chess source exposed a generic front-end defect:
  arithmetic floating constant expressions in static integer initializers
  (notably `long I = 8e3`) were emitted as zero.  Static scalar, array,
  aggregate, and compound initializers now apply the target integer conversion
  after evaluating the full floating expression, with a focused five-profile
  regression.  The canonical compiler/libc suite passes 4033/4033.
- Completed the final size-only optimization pass without changing `-Of`.
  `-Os` now folds bounds-checked constant offsets of fixed global objects into
  direct accesses, uses scratch HL as the return-address carrier when a modern
  callee-clean ABI leaves HL free, and removes repeated pair-immediate loads
  across absolute stores.  The transforms depend only on types, object bounds,
  ABI return registers, and instruction effects; they do not inspect source
  names or benchmark identity.  Across all 23 runnable z88dk integer programs,
  linked `-Os` output falls by 2,840 bytes (166,166 to 163,326 bytes, 123.5
  bytes per program on average), with every program still passing.  All 23
  `-Of` binaries remain byte-for-byte identical to the pre-pass baseline.
  Added emitted-code and execution regressions for global aggregate offsets and
  callee-clean returns; the canonical compiler/libc suite passes 4019/4019.
- Added source-independent speed optimizations for Z80 loop and data-layout
  workloads: adjacent little-endian byte packs collapse to one 32-bit load;
  proven pointer cursors, stationary aggregates, and strided word offsets can
  remain in IY; independent reductions receive disjoint register homes; and
  bounded byte induction variables use direct low-byte updates and compares.
  Relocatable symbol-plus-constant addresses are now materialized directly,
  compact-frame word equality tests compare the likely discriminating low byte
  first, and jump-table selectors can remain in A when their allocated home is
  dead.  Repeated constant-offset 32-bit loads can also use IY addressing, and
  a single-use load feeds its immediately following 32-bit addition without a
  four-byte spill/reload.  Selection uses IR shape, types, use/definition
  counts, dominance, and clobber proofs only--never source paths, symbol names,
  or benchmark identity.  Added execution and emitted-code regressions for
  each new transformation.  The canonical compiler/libc suite passes
  4012/4012, and both XCC profiles pass all 23 available z88dk integer
  programs.
- Reduced generic `-Os` output across the complete z88dk benchmark corpus by
  891 bytes (169,562 to 168,671 bytes, about 38.7 bytes per program).  The Z80
  peephole optimizer no longer expands a five-byte stack allocation into an
  equal-or-larger push sequence, and three or more framed functions can now
  profitably share the 16-byte IX enter/leave helpers.  The front end also
  preserves C `_Noreturn`, allowing `-Os` terminal direct register-argument
  calls to become jumps when the function and calling convention make that
  safe; stack arguments, interrupt functions, and critical functions remain
  calls.  All three transformations are restricted to the size profile;
  `-Of` remains unchanged and speed-oriented.  Added focused regressions for
  all three transformations.  The canonical compiler/libc suite passes 3961/3961,
  and all 23 available z88dk integer programs pass in both XCC profiles.
- Fixed four source-independent compiler defects exposed by the upstream z88dk
  `umchess.c` and Spencer-regexp torture sources: function-qualified user-label
  assembly names, integer-constant-expression evaluation for switch cases,
  invalid global post-increment rematerialization, and Boolean comparison
  normalization that was previously allowed for non-Boolean values.  Global
  byte comparisons and global bit tests now use legal Z80 load sequences, and
  `-Of` applies proven Boolean-source normalization without enabling the broader
  value-propagation policy.  Added a profile-wide execution regression covering
  these interactions.  Also added reusable scaled frame/global load residency
  and compact global-table offset allocation, reducing M-model interpbench from
  44.3M/35.6M cycles to 43.5M/31.3M (`-Os`/`-Of`) and ptrbench `-Of` from
  18.6M to 17.1M without benchmark-name or source-pattern recognition.
- Added paired Z80 allocation for reusable scaled global-array indices and
  their loaded word values: DE retains the index while HL carries the value
  across equality and ordered comparisons.  Scaled address formation now
  preserves live BC/DE state, optional constant masks coalesce in HL, and
  compact-frame signed and unsigned comparisons can consume HL directly
  without a destructive DE shuttle.  Also generalized the frameless byte
  comparison selector to the reloaded `-Os` IR form, retained safe BC loop
  values across direct no-stack-argument calls, and lowered zero-extended
  byte placement into 32-bit lanes without a general shift sequence.  These
  source-independent changes reduce M-model searchbench from 26.4M to 24.9M
  cycles in both profiles, maskbench from 29.4M to 25.5M/25.4M
  (`-Os`/`-Of`), strbench `-Os` from 22.6M to 19.7M, and MD5 from
  35.8M to 31.4M/30.8M.  The full compiler/libc suite passes 3954/3954 and
  both XCC profiles pass all 23 z88dk integer programs.
- Added source-independent three-addend fusion for 32-bit bitwise expression
  trees, forwarded proven zero-extended byte and word multiply operands at IR
  level, and selected the direct 16x16-to-32 multiply/shift-by-eight leaf form.
  The Z80 allocator now recognizes canonical constant-trip countdowns and keeps
  safe counters in BC, with in-place `dec bc` and direct BC truth tests. Also
  fixed an xopt control-flow bug that could forward DE across a label-only loop
  join. Narrow-operand/wide-result target forms are now kept terminal across
  inlining and constant evaluation, and fixed-count shift/add byte folds accept
  both explicit and forwarded widening forms. On the unchanged M-model corpus,
  CRC falls from 141.4M/120.4M to 107.1M/86.1M cycles (`-Os`/`-Of`), MD5 from
  49.8M/49.2M to
  44.0M/43.5M cycles (`-Os`/`-Of`), queenbench from 35.4M/31.3M to
  33.1M/28.9M, fixedbench from 45.7M/42.8M to 35.4M/33.9M, and histbench
  from 34.5M/33.9M to 30.3M/30.1M; all transforms are selected from IR shape,
  types, use counts, and clobber proofs rather than source or symbol names.
- Added source-independent frameless lowering for proven byte-distance,
  byte-compare, zero-terminated copy, fixed byte equality, and fixed
  shift/add/fold leaf loops. Added side-effect-free partial redundancy
  elimination for scaled indices and addresses repeated across at least three
  branch regions, with common-dominator, path-mutation, and single-definition
  proofs; `-Os` can keep one resulting repeatedly dereferenced pointer in IY.
  Terminal scalar promotion is extended only for dense dispatch functions so
  VM/parser state benefits without regressing binary-search bounds. On the
  unchanged M-model corpus, hashbench falls from 51.0M/48.6M to 31.3M/28.1M
  cycles (`-Os`/`-Of`), strbench from 29.2M/24.9M to 22.9M/19.6M,
  interpbench from 47.9M/36.6M to 44.9M/35.6M, and switchbench `-Os` from
  39.7M to 37.5M. All 23 programs remain correct in both profiles; hashbench
  is now faster than SDCC in both.
- Added a dominance-proven terminal live-range split for delayed loop-carried
  integer accumulators. Constant initializers now move to the outermost natural
  loop preheader only when that preheader dominates every update and exit use;
  zero-trip loops and side exits remain explicitly covered. The Z80 allocator
  can consequently keep the hot reduction in BC without treating unrelated
  setup loops as part of its lifetime. Also added bounded caller-save IY
  residency across direct register-argument calls without changing the public
  ABI or any stack-call layout. On the unchanged M-model corpus, listbench
  falls from 51.3M/51.3M to 44.3M/44.2M cycles (`-Os`/`-Of`), CRC from
  145.3M/124.3M to 141.4M/120.4M, and MD5 from 51.6M/51.1M to
  49.8M/49.2M; all 23 programs remain correct in both profiles.
- Enabled branch-form Boolean arithmetic in `-Of`, used repeated INC/DEC for
  small signed constants in every optimized speed profile, and added direct
  compact-frame 32-bit additions including a carry-correct four-addend chain.
  Extended lockstep pointer lowering to initialize running cursors from proven
  affine index expressions, with guards for nested induction variables,
  post-increment snapshots, and calls; call-free speed kernels use at most four
  profitable cursors.  These source-independent changes reduce M-model MD5
  from 55.0M cycles to 51.6M/51.1M (`-Os`/`-Of`), make sieve consistent at
  6.0M in both profiles, and reduce matrixbench `-Of` from 43.5M to 34.7M
  cycles (versus SDCC at 33.4M), while retaining 23/23 benchmark correctness.
- Added cost-aware BC/IY pointer-home swapping with complete temp/local
  interference checks, direct indexed-byte compare fusion, short coupled-loop
  counter narrowing, a generic `(x << n) + x + byte` accumulator fusion,
  overflow-correct direct signed comparisons between compact frame words, and
  carry-based signed subtraction of two zero-extended bytes. Enabled proven
  repeated dynamic word-load forwarding in `-Of` and CFG-liveness spill-slot
  colouring for medium-sized speed-profile frames.
  These source-independent changes reduce M-model ptrbench from
  20.1M/21.7M to 18.1M/18.6M cycles (`-Os`/`-Of`), hashbench `-Of` from
  47.4M to 44.4M, strbench from 30.0M/27.0M to 29.3M/25.0M,
  queenbench from 38.7M/33.1M to 35.4M/31.3M, matrixbench `-Of` from
  44.8M to 43.5M, and interpbench `-Of` from 44.5M to 36.6M while retaining
  23/23 correctness in both profiles.
- Reused pure local address computations, fused proven byte/word memory
  read-modify-write chains for small constant increments/decrements, compared
  adjacent byte dereferences without frame spills, and extended dead induction
  countdowns to bounded 16-bit loops.  These are source-independent IR/backend
  transforms with volatile, alias, use-count, and control-flow guards.  On the
  unchanged M-model corpus they reduce hashbench from 64.5M/53.8M to
  59.7M/50.7M cycles (`-Os`/`-Of`), histbench from 55.0M/50.3M to
  34.6M/34.0M, and listbench from 68.7M/69.7M to 51.4M/52.4M.
- Made the unified benchmark wrapper forward its selected XCC binary into the
  z88dk full-program runner and normalized relative output paths before
  per-benchmark directory changes, preventing stale compiler measurements and
  false all-compiler `HANG` reports.
- Added the current z88dk `80cc-codegen` integer corpus as a reproducible
  full-program benchmark suite.  It compares complete linked M-model binaries
  for XCC `-Os`/`-Of`, sccz80, SDCC, and both 80cc frame modes, validates every
  program in an emulator, and retains build/run logs for failures and timeouts.
- Lowered zero-extended 16x16-to-32 multiplication through the existing
  `___muluint2ulong` primitive, including a proven multiply/shift-by-eight/
  narrow fusion, and removed redundant low-bit masks before narrowing casts.
  This reduced fixedbench from 54.6M to 46.3M cycles at `-Os` and from 50.3M
  to 42.9M at `-Of` while preserving all optimization-profile regressions.
- Combined portable unsigned 32-bit rotate expressions into native IR rotates
  and lowered them as byte permutations plus at most four circular bit steps.
  Spilled 32-bit AND/OR/XOR operations now use direct IX-relative byte ALU
  forms, and large functions use CFG-liveness-coloured spill slots in every
  optimization profile.  Together these general changes reduced the MD5
  benchmark from 78.3M/52,927 bytes (`-Os`) and 108.8M/54,816 bytes (`-Of`)
  to 56.1M/33,139 bytes and 56.2M/33,254 bytes, respectively; focused rotate
  and bitwise execution regressions cover all five optimization levels.
- Improved Z80 byte-loop register allocation so IY-based scans can retain a
  loaded value and loop-carried byte accumulator in D/E across safe loop
  operations. Byte equality and byte packing now consume those register homes
  directly instead of spilling through IX or AF.
- Added execution and assembly regressions for register-resident RLE scans.
  On the 40-case portable cross-compiler benchmark, `xcc -O3` now passes all
  cases while producing 11.34% fewer payload bytes and using 4.74% fewer cycles
  than the fastest passing competitor result.
- Made register-allocation profitability loop-aware, weighting uses in nested
  loops more heavily than cold uses, and modelled B/C as independent register
  units while retaining BC interference with both bytes.
- Enabled proven one-instruction HL retention for immediate returns and near
  dereferences in non-size profiles, and enabled cross-basic-block byte/word
  load forwarding outside `-Os` only when every reachable predecessor agrees
  and conservative store/call kills preserve the available value.
- Kept pure address expressions available across read-only loads, allowing
  neighbouring field accesses to share their computed object base. Added
  source-independent linked-cursor allocation for pointers advanced through a
  loaded link, including safe resets to an invariant head for repeated walks.
- Extended lockstep pointer lowering to subsume proven secondary byte-offset
  induction variables and to accept bounded, call-free walks over invariant
  incoming near pointers. The proof requires one constant initialization, one
  constant self-step, and address-only uses before removing the redundant
  offset chain.
- Lowered signed 16-bit `< 0` and `>= 0` branches to direct sign-bit tests.
  Added all-profile execution coverage for linked and bounded pointer walks,
  two-pointer comparisons, repeated cursor resets, and signed extrema.
  On the unchanged full-program corpus these general changes reduced
  structbench to 6.9M/6.7M cycles (`-Os`/`-Of`), ptrbench to 22.5M/24.1M,
  listbench to 68.7M/69.7M, queenbench to 45.4M/39.2M, and hashbench to
  64.5M/53.8M; all 23 XCC rows pass in both profiles.
- Extended `xopt` to remove short EXX regions whose bodies cannot observe or
  modify BC/DE/HL, with positive and live-register rejection tests.
- Serialized the initial shared-host-library build for parallel tool builds
  and made archive output directories order-only prerequisites, preventing
  concurrent writers from corrupting `libxz80.a` and related archives.

## v1.7.1 - Beta - 2026-06-20

- Completed the `x/tests/c23` `xcc-z80` setup by adding real helper scripts:
  `x/tests/c23/setups/xcc-z80/bin/xcc_z80_driver.sh` and
  `x/tests/c23/setups/xcc-z80/bin/run_xcc_z80.sh`.
- Kept the dedicated `x` release pipeline that builds and publishes `x.tar.gz`.
- Preserved the `xcc`, `xas`, `xar`, `xld`, `xobjcopy`, `xopt`, `xgdb`, and
  `xemu` tool set introduced in earlier alpha tags.

## v1.7.0 - Alpha - 2026-06-20

- Updated the `x/tests/c23` `xcc-z80` profile to pass an explicit compiler path
  into its driver template.
- Kept the dedicated `x` release workflow that archives only `bin/x` as
  `x.tar.gz`.
- The helper scripts referenced by the `xcc-z80` profile were still not present
  in this tag.

## v1.6.0 - Alpha - 2026-06-20

- Added an automated MI smoke test for `xgdb`:
  `x/src/xgdb/tests/mi_smoke_test.sh`.
- Switched release automation from a monolithic `xyz-release.tar.gz` payload to
  a dedicated `x.tar.gz` build that stages only the `x` distribution.
- Updated the release workflow to the `xgdb` VS Code extension path and the
  dedicated `bin/x` packaging layout.

## v1.5.0 - Alpha - 2026-06-20

- Split staged output into distinct `bin/x`, `bin/y`, and `bin/z` prefixes,
  making the `x` toolchain a relocatable install tree with its own manuals and
  packages.
- Added `xobjcopy` as an object/archive conversion tool and `xopt` as a
  standalone Z80 assembly optimizer.
- Expanded `xld` beyond the earlier SDCC-only flow: GNU ELF object input,
  GNU/SDCC archive reading, linker scripts, map output, derived ELF plus
  DWARF sidecars in GNU mode, and documented Intel HEX output all appear in
  the staged manuals and sources.
- Expanded `xas` with macro processing and source-to-source dialect conversion
  between SDCC-style and GNU-style assembly.
- Grew `xcc` substantially with fixed-point float runtime families
  (`fixed8_8`, `fixed16_16`, `fixed24_8`), many new optimizer regressions, and
  broader ABI/runtime tests.
- Expanded the target libc surface with `stdio.h`, `threads.h`, `fcntl.h`,
  `unistd.h`, `sys/stat.h`, `sys/types.h`, plus large additions in complex
  math, transcendentals, wide-character support, and thread runtime code.
- Added the repository-wide `x/tests/c23` compatibility suite, including an SDCC
  setup and an `xcc-z80` profile skeleton. In this tag the `xcc-z80` profile
  already exists, but its helper scripts are not yet present.
- Added staged tool manuals under `x/docs/dist/man/` for `xar`, `xas`, `xcc`,
  `xgdb`, `xld`, `xobjcopy`, and `xopt`.

## v1.4.0 - Alpha - 2026-06-07

- Marked the transition from the earlier `xlink`/`xdbg`-centric stack to a
  staged `xc` toolchain built around `xcc`, `xas`, `xar`, `xld`, `xgdb`, and
  `xemu`.
- Added a large assembler-based libc surface under `x/libc`, including new
  headers and implementations for `assert`, `complex`, `ctype`, `errno`,
  `fenv`, `inttypes`, `locale`, `math`, `setjmp`, `signal`, `stdlib`,
  `string`, `time`, `uchar`, `wchar`, and `wctype`.
- Restructured project documentation around `docs/README.md`, standards,
  how-to guides, and staged distribution docs.
- Started staging the toolchain as an explicit host-plus-target slice with
  host executables and target headers/libraries.

## v1.3.0 - Alpha - 2026-05-24

- Improved `xdbg` source-less debugging: functions without resolvable source
  files now fall back to symbol/disassembly views instead of being attached to
  the wrong source file.
- Added DAP disassembly handling in `xdbg`, including advertised
  `supportsDisassembleRequest` capability and byte-accurate instruction output.
- Added `stepOut` support in the DAP path by continuing to a temporary
  breakpoint at the stack return address.
- Improved debugger integration guidance for emulator authors, including the
  `server.close()` shutdown path and the separation between target state and
  frontend source lookup.
- Refined `xlink`-generated `.xdbg` output for library functions that have
  symbol ranges but no resolved source file on disk.

## v1.2.0 - Alpha - 2026-05-23

- Expanded `xlink` substantially with new parsers and emitters for `.adb`,
  `.cdb`, `.lst`, NoICE output, linked `.xdbg` output, runtime injection, and
  library archive handling.
- Added tests for the new `xlink` capabilities, including runtime, CDB, NoICE,
  library parsing, and `.xdbg` emission paths.
- Documented `xlink` as a linker that can selectively pull from libraries,
  emit `XL` and `BIN`, generate NoICE files, produce linked SDCC debug data,
  and consume optional sidecars from compiler and assembler builds.

## v1.1.0 - Alpha - 2026-05-17

- Kept the same executable surface as `v1.0.0`, centered on `xlink`,
  `xdbg`, and `xdbg-z80`, plus the supporting host utilities.
- Added debugger integration documentation to the packaged docs set.
- Updated staged distribution documentation and Debian packaging metadata.

## v1.0.0 - Alpha - 2026-05-17

- Initial tagged `x` baseline with a staged distribution centered on `xlink`,
  `xdbg`, and `xdbg-z80`.
- Shipped host-side support libraries including `libxdbg.a`, `libxdbg_cli.a`,
  `libxdbg_dap.a`, `libxdbg_mi.a`, and `libxdbgstub.a`, alongside the
  repository's other host tools.
- Staged public debugger headers and the host-side debugger support libraries
  in the `xgdb` include tree.
- `xlink` already handled SDCC-style `.rel` and `.lib` inputs, demand-driven
  library inclusion, `XL` and flat `BIN` output, and SDCC v1 plus `XL4`
  record parsing.
- `xdbg` already exposed CLI, MI, and DAP frontend code, plus a reference Z80
  remote target.
- Early `xas` and `xcc` source trees were already present in the repository,
  but the staged distribution was still focused on the linker/debugger stack
  rather than the later full compiler-suite packaging.
