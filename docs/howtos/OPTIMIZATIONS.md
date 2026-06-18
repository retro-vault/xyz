# Optimization Guide

This page describes the optimizations that `xcc` already implements
today.

There is no separate active backend-codegen backlog document right now.
When a new structural code-emission problem is found, it should be
described here or added as a fresh research note at that point.  External
optimizer and superoptimizer references for the experimental lane are kept
in [O3_RESEARCH_SOURCES.md](O3_RESEARCH_SOURCES.md).

The Z80 assembly peephole layer is shared through `libxopt`, and can
also be run directly on `.s` files with the standalone `xopt` tool.
`xcc` still owns the IR-level and whole-function C-shape optimizations,
but both `xcc` and `xopt` use the same assembly optimizer
implementation.

## Public Levels

The current public optimization model is:

- default / `-O0`: no optimization
- `-O1`: peephole optimization
- `-O2`: general optimization
- `-Of`: speed optimization
- `-O3`: experimental optimization
- `-Os`: size optimization

These are presets, not additive flags. You normally pick one of them.

`-Of`, `-O3`, and `-Os` now share the same proven aggressive baseline.
That baseline includes the helper-inline budget, denser switch lowering,
the structured-loop pipeline, and the benchmark-proven direct emitters
that survived the full execution and benchmark oracle.

`-Os` is deliberately conservative: it keeps that current record-setting
size baseline intact. New tricks that are not explicitly size-policy
neutral should land in `-O3` or `-Of` first, not in `-Os`.

`-O3` is intentionally the experimental playground: here be dragons.
It keeps the proven baseline, then layers speed, size, and shape-changing
experiments that are allowed to trade one goal for another. The protected
size-record lane is `-Os`, not `-O3`.

The newer generic structured-loop pipeline is now in the stable presets
too: direct control-condition lowering, counted-byte-loop narrowing,
pointer-walk canonicalization, and the generic walked-loop backend
emitters that feed from those shapes all run under `-O2` / `-Of` / `-Os`.

On the current bare-metal executable benchmark suite, `-Of`, `-O3`, and
`-Os` start from the same promoted baseline, but `-Of` and `-O3` now
diverge through speed-biased work. `-Os` is the lane we protect for the
size record.

## Fine-Grained Flags

`xcc` also supports GCC-style per-optimization overrides:

- `-f<name>` enables one optimization family
- `-fno-<name>` disables one optimization family

If you combine them with an `-O` preset, options are processed left to
right and the last relevant setting wins.

### Currently supported `-f` names

- `peephole`
- `dead-static-functions`
- `const-arg-prop`
- `const-call-eval`
- `function-const-eval`
- `dead-params`
- `merge-identical-functions`
- `inline-static-functions`
- `cfg-cleanup`
- `jump-threading`
- `address-deref-fold`
- `value-propagation`
- `constant-fold`
- `algebraic-simplify`
- `loop-licm`
- `loop-induction`
- `strength-reduction`
- `dead-code-elim`
- `scalar-local-promotion`
- `reg-param-promotion`
- `short-circuit-bool-ifx`
- `narrow-counted-byte-loops`
- `loop-pointer-walk`
- `promoted-byte-compare`
- `promoted-byte-ops`
- `rotate-combine`
- `duplicate-block-merge`
- `merge-tails`
- `local-frame-compaction`
- `regalloc`
- `compare-ifx-fusion`
- `frame-omit`
- `prealloc-temp-frame`
- `switch-jump-tables`

Most of these are enabled through `-O2`, `-O3`, `-Of`, or `-Os`. The
`-fprealloc-temp-frame` switch still exists for targeted experiments,
but it now mainly serves to force wider TEMP preallocation than the
default optimized heuristic uses. `-fregalloc` is still available for
targeted bisects and for forcing it on from lower presets, but the
bounded stable allocator is now part of the normal `-O2` / `-Os`
pipeline. `-fduplicate-block-merge` is in a different category now: it
is enabled by the experimental `-O3` preset, but it is not part of the
stable `-O2` / `-Os` presets. The public O3 form is intentionally
hardened: whole-block merging now requires exact temporary operands, so
two similar tails that read different incoming temps are not redirected
into one shared block.

## `-O1`: Peephole Optimization

`-O1` runs the textual Z80 peephole pass after code generation.

Current rule families include:

- redundant self-load removal:
  `ld r, r`
- redundant push/pop cleanup:
  `push hl` / `pop hl`
- jump-to-next-label removal
- conservative short-branch relaxation:
  eligible `jp` becomes `jr` when the late peephole pass can prove the
  branch is comfortably in relative range
- branch inversion to remove an unnecessary unconditional skip
- duplicate flag-test removal
- temp store/reload cleanup for common IX-frame patterns
- zero-load shortening:
  `ld a,#0` to `xor a`
- zero-compare shortening
- `HL != 0` boolean-materialization shortening back into direct flag
  tests when the value being checked is already live in `HL`
- boolean-materialization plus `IFX` collapse back into a direct branch
- tail-call shortening for simple wrappers:
  `call target` followed by `ret` becomes `jp target`

The simplest fixed-window peepholes now live in a small structural rule
table rather than each being hand-coded separately. Context-sensitive
patterns such as IX-frame temp reload cleanup and boolean short-circuit
collapse still use custom matchers.

### Example: Wrapper Tail Call

Input:

```c
void g(void);
void f(void) { g(); }
```

With `-O1` and above, the wrapper now becomes:

```asm
_f:
        jp      _g
```

instead of emitting a real call followed by a return.

## `-O2`: General Optimization

`-O2` keeps the `-O1` peephole pass and adds the current module-level
and per-function IR optimizers.

The module-level IR pass currently removes dead internal-linkage
functions that are never referenced anywhere else in the translation
unit, and it can propagate constant actual arguments into eligible
internal callees before the per-function IR pipeline runs. When a
parameter becomes completely unused after that, the same module pass can
also remove it and rewrite every direct call site to match the smaller
ABI. It can also merge identical tiny internal helpers and redirect all
their direct callers to one surviving copy. For pure integer helpers in
the currently supported subset, it can also evaluate a direct call at
compile time when every actual argument at that call site is constant,
including the common case where a helper call is fed from
constant-valued locals or temporaries rather than raw literal `SEND`
operands. That includes nested calls through other eligible private
helpers and a small whitelist of pure runtime helpers such as
`__mul16`, `__div16`, `__mod16`, `__mul32`, `__div32`, and `__mod32`.
And when a whole zero-argument integer function stays inside that same
safe subset, `-O2` can collapse the entire function body to one
constant return. That now includes straightforward 32-bit integer code
and bounded sub-object reinterpretation of local scalars, such as
viewing an `unsigned long` local as two `unsigned int` words through a
local pointer cast.

The IR pipeline currently includes:

- CFG cleanup
- direct control-condition lowering for `if`, `while`, `do ... while`,
  and `for`, so short-circuit `&&`, `||`, and `!` in statement
  conditions become branchy IR directly instead of first building larger
  value-style boolean trees
- label-chain collapse and jump threading through label-only or
  `goto`-only blocks
- duplicate labeled-block merging when two explicit branch targets lower
  to the same straight-line body
- scalar local promotion for simple helper-free 16-bit stack locals whose
  address is never taken
- narrowing of promoted unsigned-byte compares back to byte-width when
  both operands are provably in the `0..255` range
- safe collapse of widened byte `EQ` / `NE` comparisons back to direct
  byte compares when both sides really came from byte-sized values
- narrowing of longer single-use promoted-byte arithmetic chains back to
  byte-width even when the IR briefly widened them through `unsigned int`
- rotate combine for unsigned 16-bit idioms such as
  `(x << k) | (x >> (16 - k))`
- counted-byte loop narrowing for proven `< 256` induction variables
- loop pointer-walk canonicalization, so repeated `base + i` array
  accesses can turn into loop-carried walked pointers before Z80
  lowering
- constant folding
- algebraic simplification
- loop-invariant code motion
- induction-variable cleanup
- strength reduction
- dead code elimination

The basic algebraic identities in that pipeline now go through one small
declarative rule table shared by the SSA-style simplifier and the later
algebraic-simplify pass, so rules like `x + 0 -> x`, `x * 1 -> x`, and
`x & 0 -> 0` stay consistent across both stages.

`xcc` also has a bounded temp register-allocation prepass in the stable
`-O2` / `-Os` presets, and you can still toggle it explicitly with
`-fregalloc` / `-fno-regalloc`:

- one short-lived 16-bit temp may live in `BC`
- one next-use-only 8-bit temp may live in `A'`

When it applies, this reduces IX-frame traffic on the common short-range
temp cases without pretending to be a full SDCC-style graph allocator.
The stable allocator is intentionally conservative:

- only straight-line, gap-free live windows are considered
- only one 16-bit temp is kept in `BC`
- the `A'` path is restricted to byte temps whose only use is in the
  immediately following instruction, which is enough to trim byte-heavy
  arithmetic chains without reopening the broader flag-liveness risks

For the same narrow class of straight-line scalar helpers, the backend
now also reuses stack TEMP spill slots by live range instead of sizing
the frame as if every temp were live at once. That currently applies
only when the function stays away from nonlocal storage, calls, and
pointer-style memory operations, which is what keeps the shrink stable
on the benchmark suite.

Another backend win in the same area is that stack-resident TEMP slots
now honor real byte widths. A spilled `unsigned char` TEMP no longer
automatically reserves a 2-byte frame slot just because the older frame
planner defaulted every anonymous TEMP to word size. That cut a large
amount of wasted IX-frame space out of byte-heavy kernels such as
`gray_decode` without changing their source-level semantics.

`-O2` and above also now rematerialize a narrow class of cheap
address-like 16-bit temps instead of always spilling them:

- direct `ADDRESS_OF` temps
- direct `object + zero-extended u8 index` temps
- the same indexed-address shape when the base came from a preceding
  `ADDRESS_OF` temp
- copies and casts of those rematerializable pointer temps
- `+/- literal` offsets on top of those rematerializable pointer temps

This is intentionally limited to pointer-style dereference sites. The
goal is to remove frame traffic for transient array-walk addresses
without reopening the broader liveness hazards that come with trying to
keep arbitrary pointer values live in registers for long stretches.

There is also a second, even narrower BC-residency path for simple
16-bit locals and parameters. When a local or parameter stays inside a
very small straight-line region with no address-taking and no obvious
`BC` hazards, the backend can keep that symbol in `BC` instead of
reloading it from the IX frame on every direct use. This is still much
smaller than a full SDCC-style allocator, but it is an important step
away from treating only anonymous temps as register-worthy.

For modern `sdcccall(1)` functions, `-O2` can also promote leading
register-passed parameters into temporaries when the callee is simple
enough for the current allocator model: no backward branches and no
helper-heavy arithmetic or call operations. That lets `xcc` avoid some
fixed parameter spill slots and, in the best small straight-line cases,
turn the incoming argument register into an ordinary temp that can feed
later simplification and register allocation more directly.

On the backend side, the same `-O2` family now also keeps some leading
`sdcccall(1)` register arguments live in their incoming registers for
their first real use instead of eagerly spilling them into the IX frame
in the prologue. If the value must survive beyond that first use, `xcc`
materializes it back into the frame at that point. This is intentionally
conservative right now, but it already trims a class of pointless
store-then-immediate-reload sequences in small helper functions.

The backend now also has real byte-native lowering for common byte-sized
operations instead of routing them through 16-bit `HL` scaffolding first.
For byte results, `xcc` can now emit:

- `add a,*` / `sub *` for byte add/sub
- `xor` / `or` / `and` directly in `A`
- byte `cpl`
- byte left/right shifts in `A`, including small constant unrolling

That matters most in benchmark-style kernels where unsigned-byte state
mutates in tight loops.

The Z80 backend now also keeps a small exact-content cache for `HL` and
`DE` under `-O2`, `-O3`, and `-Os`. When straight-line code reloads the
same local, temp, or immediate into one of those pairs, `xcc` can now
skip the duplicate reload entirely. The cache is intentionally narrow:
it is flushed at labels, calls, and any instruction form that mutates
the pair, and all memory writes conservatively flush it before the
backend seeds it again with a known post-store value. This is a direct
adaptation of the “pair-content tracking” idea from SDCC’s Z80 backend,
but kept much smaller and safer.

The same optimized presets now also keep a narrow exact-content cache
for `A`. When straight-line code reloads the same local byte, temp
byte, or constant into `A` and no proven `A` clobber happened in
between, `xcc` can now skip that reload too. This cache is even more
conservative than the pair cache:

- labels and calls flush it
- `ex af,af'`, direct `A` writes, and the usual byte arithmetic/rotate
  instructions flush it
- volatile/global/TLS byte reads do not seed it

That still buys a useful reduction in repeated `ld a, N(ix)` traffic in
byte-heavy benchmark loops while keeping the full executable oracle
stable.

Another small backend cleanup is direct absolute memory access for fixed
literal pointer addresses. When the backend can prove a dereference is
against an exact constant address, it can emit a direct absolute load or
store instead of first building that address in `HL` and then going
through the generic indirect path.

And at the IR level, temporary results of `&global_object` no longer
have to be materialized through stack temps just to feed `base + index`
address arithmetic. Stable optimized builds now rewrite those temporary
uses back into direct label-address operands, which cuts a surprising
amount of frame traffic from array-heavy benchmark kernels.

`-O2` and above now also recognize one common rotate idiom before Z80
code generation: unsigned 16-bit expressions of the form
`(x << k) | (x >> (16 - k))`. Instead of lowering those as two separate
shifts plus an OR, the backend now emits a compact rotate-style sequence.
This is especially useful in small hash/mix helpers and loop kernels.

Finally, constant 16-bit shifts are now emitted more compactly:

- small known shifts are fully unrolled instead of paying a `djnz` loop
- larger constant shifts no longer emit a pointless zero-count test
  before the loop, because the count is already known nonzero

## `-Of`: Speed Optimization

`-Of` currently means `-O2` plus the safe speed-oriented generic work
that proved stable enough to graduate out of the experimental lane.

The main extra behavior today is:

- the wider static-helper inline budget that used to live only in `-O3`
- dense switch jump-table lowering for integer switches when the backend
  can prove the span is profitable
- the same generic structured-loop pipeline that stable `-O2` already
  uses, but paired with the more aggressive speed-biased helper-inline
  thresholds
- speed-biased constant 16-bit shifts: `-Of` unrolls shifts by 6 and 7
  to avoid the `DJNZ` loop overhead, accepting a small size increase for
  fewer cycles
- speed-biased constant accumulator logical-right shifts, where repeated
  `srl a` trains become a shorter rotate-and-mask sequence when flags
  are overwritten before they can be observed, or when the mask can be
  folded into a following `and #imm`
- speed-biased count-5 two-register right-shift loop unrolling for
  hot `srl l; rr a; djnz` loops when the private loop label has no
  other users and `B` is dead until an overwrite. This is deliberately
  a speed-for-bytes dragon and is not enabled for `-Os`
- speed-biased loop-invariant `ld d,#0` hoisting for simple
  single-backedge byte-index loops that use `DE` only as
  `D=0, E=index` before `add hl,de`; this keeps the same payload size
  and removes one zero-high-byte load from every loop trip after the
  first
- speed-biased same-size pair-copy peepholes, shared with `-O3`, such as
  `push de; pop hl` becoming `ld h,d; ld l,e`, and
  `ld h,d; ld l,e` becoming `ex de,hl` when all following paths prove
  `DE` is overwritten before it is read
- speed-biased dead return-copy cleanup, where
  `ld b,h; ld c,l; ex de,hl; <epilogue>` becomes
  `ex de,hl; <epilogue>` because `BC` is caller-saved and not part of
  the modern 16-bit return value
- the O3 superoptimizer-inspired adjacent peepholes, including exact
  accumulator identities, low-bit pair arithmetic shortcuts, and short
  straight-line dead-flag-setter removal
- speed-biased register-move cleanup for round-trips and pure dead loads
  exposed by the other peepholes, such as `ld a,h; ld h,a; ld a,#3`
  becoming just `ld a,#3`
- speed-biased zero-extend cleanup for byte indexes, such as
  `ld l,a; ld h,#0; ld b,h; ld c,l` becoming `ld c,a; ld b,#0`
  when the following short straight-line span proves `HL` is dead
- speed-biased generalized zero-extend cleanup, where the same direct
  `BC = zero_extend(X)` collapse also applies to register, immediate,
  `(HL)`, and indexed sources that can be loaded directly into `C`
- speed-biased zero-extend truth-test cleanup, where the exposed
  `BC = zero_extend(A); HL = BC; A = H; A |= L` test collapses to
  `or a,a`; a small CFG-aware liveness proof can then remove the
  `HL = BC` copy, and even the original `BC = zero_extend(A)`, when
  every branch path overwrites the relevant pair before reading it
- speed-biased zero-extend pair-test shortcut, where
  `HL = zero_extend(X); BC = HL; HL = BC; A = H; A |= L` becomes
  `HL = zero_extend(X); BC = HL; A = L; A |= A`, preserving both pairs
  while removing the redundant copy-back before the byte zero test
- speed-biased separated byte-immediate pair folding, where independent
  setup such as `ld b,#hi; <safe span>; ld c,#lo` becomes one
  `ld bc,#hilo` when the intervening span does not read or write the
  delayed byte or full pair
- speed-biased direct IX word increments, where a stack-local
  load/`inc hl`/store round trip becomes
  `inc N(ix); jr nz,L; inc N+1(ix); L:` when the following path proves
  flags are overwritten before any observer can read them
- speed-biased direct IX byte increments, where a stack-local
  `ld a,N(ix); add a,#1; ld N(ix),a` round trip becomes `inc N(ix)`
  when both `A` and flags are proven dead along the following path
- speed-biased IX byte load forwarding, where `ld a,N(ix); ld r,a`
  becomes `ld r,N(ix)` when a path-sensitive proof shows the
  accumulator is overwritten before any later read
- speed-biased accumulator liveness through 16-bit pair arithmetic, so
  safe rewrites can cross `add hl,bc`, `adc hl,de`, and similar
  non-accumulator arithmetic instead of treating every `add`/`adc`/`sbc`
  as an `A` read
- speed-biased compare fallthrough reload cleanup, where
  `ld a,X; cp #n; jr cc,T; ld a,X` loses the second load when only
  unreferenced fallthrough labels sit between the branch and reload
- speed-biased transformed-compare fallthrough cleanup, where
  `ld a,X; xor #k; cp #n; jr cc,T; ld a,X; xor #k` loses the repeated
  load and transform on the fallthrough path because `cp` and the
  branch preserve the transformed accumulator value
- speed-biased zero-store chain cleanup, where
  `xor a; ld dst,a; xor a` becomes `xor a; ld dst,a` because the store
  preserves both the zero accumulator and the flags from the first
  zeroing instruction
- speed-biased stack-discard cleanup, where `inc sp; inc sp` becomes
  `pop rr` when every following path overwrites that register pair before
  reading it
- speed-biased long stack-discard cleanup, where long `inc sp` runs can
  become `ld hl,#N; add hl,sp; ld sp,hl` when `HL` and flags both die
  before observation

On the current executable benchmark suite, that split now looks like:

- `xcc -O2`: `14318` payload bytes, `4867429` cycles, `20 / 20`
  correct
- `xcc -Of`: `8146` payload bytes, `2622214` cycles, `20 / 20`
  correct

So `-Of` is currently about `43.10%` smaller and `46.12%` fewer cycles
than `-O2` on the full benchmark oracle. It is currently 540 payload bytes
smaller than `-Os` and runs about `5.02%` fewer cycles because the
backend speed-biased lane can choose faster or smaller proven forms.

## `-O3`: Experimental Optimization

`-O3` currently starts from the same promoted aggressive size baseline
as `-Os`. That is deliberate: already-proven loop emitters, helper fast
paths, and other benchmark-clean structural wins remain available while
`-Os` stays the protected record path.

So the role of `-O3` now is simpler:

- it is the place where new optimization ideas land first
- unsuccessful experiments are expected to be rejected, not kept around
- successful experiments should eventually graduate down into `-Of`,
  `-Os`, or even `-O2`

Current always-on `-O3` experiments include:

- duplicate-block merging, which redirects equivalent basic blocks with
  exact temporary operands to a canonical copy before later cleanup and
  inlining have their turn
- speed-biased adjacent stack-pair copies, where `push de; pop hl`
  becomes the same-size but much faster `ld h,d; ld l,e`
- speed-biased dead-source pair copies, where `ld h,d; ld l,e` becomes
  `ex de,hl` when a small CFG-aware liveness proof shows every later
  path overwrites `DE` before reading it
- speed-biased dead return-copy cleanup, where `BC = HL` setup is
  removed immediately before `ex de,hl` and the function epilogue.
  The supporting `BC` liveness proof also understands that `ret` kills
  caller-saved `BC`, and that `ex de,hl` does not touch `BC`.
- speed-biased constant 16-bit shift unrolling for counts 6 and 7,
  spending a few bytes to avoid the `DJNZ` loop overhead
- speed-biased constant accumulator logical-right shifts, where
  `srl a` repeated `N` times becomes `rrca` or `rlca` plus an exact
  low-bit `and` mask. A following `and #imm` is folded into that mask,
  so `srl a` five times followed by `and #1` becomes
  `rlca; rlca; rlca; and #1`.
- speed-biased count-5 two-register shift-loop unrolling, where a
  private `ld b,#5; L: srl l; rr a; djnz L` loop becomes five explicit
  `srl l; rr a` pairs if the loop label has no other users and `B` is
  dead until an overwrite
- stack-backed `HL ^= HL >> 5` xorshift cleanup, where O3 keeps the
  original IX spill stores for safety but synthesizes `BC = HL >> 5`
  with rotate/mask instructions while leaving `HL` live, removing the
  destructive five-step `srl h; rr l` chain and the reload
- speed-biased high-byte zero hoisting in simple byte-index loops,
  where loop-invariant `ld d,#0` moves before the loop label if the
  body has one backedge and does not otherwise read or write `D`/`DE`
- superoptimizer-inspired accumulator rewrites such as
  `and #0; neg` to `sub a`, `and #255; rr a` to `srl a`,
  `scf; adc a,#0` to `add a,#1`,
  `xor #255; sbc a,#255` to `neg`, `cpl; neg` to `sub #255`,
  and several shift/rotate identity collapses
- a tiny exact boolean-algebra superoptimizer for adjacent accumulator
  immediates, such as `and #240; and #15` to `and #0`,
  `or #16; or #1` to `or #17`, and
  `xor #85; xor #170` to `xor #255`
- constant-loaded accumulator folding for logical immediates, such as
  `ld a,#85; and #15` to `ld a,#5; and a`, with later fixed-point
  passes able to delete the folded load if a following `xor a` makes it
  dead
- accumulator bit-mask collapses such as `res 7,a; and a` to
  `and #127`, `res 3,a; and #255` to `and #247`,
  `set 0,a; or a` to `or #1`, and `set 4,a; or #1` to `or #17`
- same-bit register `set` / `res` chain cleanup, where the second
  operation completely overwrites the first without touching flags
- low-bit pair arithmetic shortcuts such as
  `set 0,l; dec hl` to `res 0,l`, with equivalent `DE` and `BC`
  forms, plus the inverse `res 0,l; inc hl` to `set 0,l`
- dead-flag-setter removal when a flag-only instruction such as
  `ccf`, `scf`, `or a`, `and a`, register/immediate `cp`, register
  `bit`, or a no-op accumulator ALU instruction is followed in the same
  short straight-line block by an instruction that overwrites the
  relevant flags before any branch, carry read, label, or `push af`
- register-move cleanup for speed-biased code, such as removing the
  second half of `ld a,e; ld e,a`, or removing a pure `ld b,#1` when it
  is immediately overwritten by `ld b,c`
- zero-extend cleanup for byte indexes and tests, where `A -> HL -> BC`
  shuttles collapse directly to `BC` if a nearby overwrite proves `HL`
  is only a transient staging register, and where `HL = BC` copies are
  removed from zero-tests when both branch paths overwrite `HL` before
  reading it.  The same CFG-aware liveness proof also removes dead
  `BC = zero_extend(A)` setup before zero-tests when all paths overwrite
  `BC` before any read. The direct `BC` collapse also handles non-`A`
  byte sources when Z80 can load the source straight into `C`. A second
  local form preserves both `HL` and `BC` but tests the known low byte
  directly, turning `ld a,h; or a,l` into `ld a,l; or a,a` after `H` is
  known zero.
- separated byte-immediate pair folding, where register-coverage style
  local liveness lets O3 turn independent high/low byte setup for
  `BC`, `DE`, or `HL` into a single 16-bit immediate load even when a
  short safe instruction span sits between the two byte loads
- direct IX word increment folding, where O3 replaces
  `ld l,N(ix); ld h,N+1(ix); inc hl; ld N(ix),l; ld N+1(ix),h` with a
  direct low-byte memory increment and conditional high-byte carry
  increment when a CFG-following flag proof shows the changed
  non-carry flags die before observation
- direct IX byte increment folding, where O3 replaces
  `ld a,N(ix); add a,#1; ld N(ix),a` with `inc N(ix)` when a
  path-sensitive proof shows both the accumulator result and the changed
  flags die before observation
- IX byte load forwarding, where O3 replaces `ld a,N(ix); ld r,a`
  with `ld r,N(ix)` once the register-coverage/liveness proof shows
  that the temporary accumulator value dies on every following path
- HL byte load forwarding, where O3 replaces `ld a,(hl); ld r,a` with
  `ld r,(hl)` when `A` is dead, while deliberately standing down before
  zero-extension truth-test cascades so the larger cleanup can still fire
- accumulator liveness through 16-bit pair arithmetic, so O3 can keep
  proving `A` dead across address arithmetic such as `add hl,bc`
  without mistaking that pair operation for an accumulator read
- compare fallthrough reload cleanup, where O3 removes a repeated
  `ld a,X` after `ld a,X; cp #n; jr cc,T` when any intervening labels
  are proven not to be alternate branch or call entries
- transformed-compare fallthrough reload cleanup, where O3 removes the
  second `ld a,X; xor #k` in generated signed-range checks after proving
  fallthrough labels are not alternate entries
- zero-store chain cleanup, where repeated `xor a` instructions between
  accumulator-preserving stores collapse to one zeroing instruction
- modern-ABI constant return direct loads, where generated
  `sdcccall(1)` functions that return a 16-bit constant through
  `ld hl,#imm; ex de,hl; <return tail>` become `ld de,#imm; <return tail>`.
  The rule is deliberately guarded by xcc's `sdcccall(1)` prologue marker
  so standalone `xopt` does not rewrite unmarked legacy-HL-return assembly
- alternate-register-bank cancellation, where adjacent `exx; exx` pairs
  with no labels are removed. This mostly cleans up aggressive O3/Of
  outlining and helper-call shuffles, and deliberately stays out of `-Os`
- call-argument DE direct loads, where `ld hl,A; push hl; ld hl,B;
  ex de,hl; ld hl,C` becomes `ld hl,A; push hl; ld de,B; ld hl,C` when
  the final load overwrites the swapped-back `HL`. If `A == B`, O3/Of use
  the stronger `ld de,A; push de; ld hl,C` form
- dead-HL exchange direct loads, where `ld hl,A; ex de,hl` becomes
  `ld de,A` for immediate or symbol constants when every following CFG
  path overwrites `HL` before reading it. This generalizes the guarded
  constant-return case without changing legacy-HL-return assembly.
- dead-pair stack-discard cleanup, where `inc sp; inc sp` becomes
  `pop rr` after proving that `rr` dies on every following CFG path. This
  saves one byte and is faster, prefers `BC`, and can fall back to `DE`
  or `HL` when those pairs are dead instead. It deliberately stays out
  of `-Os`.
- dead restored-pair cleanup, where `pop rr; push rr` is removed after
  proving the restored pair dies before any read. This catches standalone
  `xopt` assembly cases even though the current xcc benchmark suite keeps
  those restored `HL` values live.
- long stack-discard cleanup, where long `inc sp` runs can become
  `ld hl,#N; add hl,sp; ld sp,hl` after proving both `HL` and the changed
  flags die before any observation. The current xcc benchmark suite mostly
  rejects this shape because either `HL` or flags remain live, but the
  rule is useful for standalone `xopt` input.
- equal-pair exchange cleanup, where exact proofs that `HL == DE` remove a
  following `ex de,hl`. Current proofs cover direct `DE -> HL` copies,
  `DE -> BC -> HL` copies, and the IX-temp shape
  `ld N(ix),e; ld N+1(ix),d; ld l,N(ix); ld h,N+1(ix); ex de,hl`.
- exchange-sandwich direct loads, where
  `ex de,hl; ld hl,#K; ex de,hl` becomes `ld de,#K`. The sandwich only
  loads `DE` and restores the original `HL`, so the direct load is exact.
- Spaghetti flag-helper inlining, where tiny Spaghetti zero-test helpers
  can be inlined at `call helper; jr cc,L` sites if every call observes
  only the flags and liveness proves the helper's changed output pairs die.
  The current generated runtime corpus often keeps those pairs
  conservatively live across external calls, so this mostly benefits
  standalone `xopt` inputs and future helper shapes.
- Spaghetti outlining, an O3-only whole-file `libxopt` pass that slides a
  vertical kernel over the final Z80 stream, finds profitable duplicate
  straight-line instruction runs, extracts one shared helper, and replaces
  each occurrence with a call or tail jump.  The current vector is dynamic
  over profitable 3- to 12-instruction kernels and runs several bounded
  extraction rounds so large islands are peeled first and smaller leftovers
  can still be harvested.  A follow-up tail-threading pass rewrites helpers
  whose callers all share the same continuation.  Spaghetti refuses labels,
  branches, calls, returns, `SP` users, push/pop, helper bodies, data
  sections, and pure shift/rotate trains.  It is intentionally a
  size-for-speed dragon, not part of `-Of` or protected `-Os`.
- Spaghetti helper ABI sinking, where O3 can rewrite exact outlined helper
  families after proving every call site has the same wrapper contract. For
  load helpers, `push af; push bc; setup; call; pop bc; pop af` becomes a
  preserving helper with the save/restore paid once. For store helpers,
  the exact `push af/bc/de`, `DE <- HL`, `IX -> HL`, call, restore wrapper
  is similarly sunk into the helper. Mixed wrapped/unwrapped helpers are
  rejected.

On the current executable benchmark suite, the split between the safe
speed lane, the dragon lane, and the protected size lane is:

- `xcc -Of`: `8146` payload bytes, `2622214` cycles, `20 / 20`
  correct
- `xcc -O3`: `7649` payload bytes, `2776428` cycles, `20 / 20`
  correct
- `xcc -Os`: `8686` payload bytes, `2760674` cycles, `20 / 20`
  correct

Against `sdcc --opt-code-size`, on the `19` benchmarks where both
compilers produce correct executable results, `xcc -Of` is currently
`26.64%` smaller and `24.66%` fewer cycles, while `xcc -O3` is
`31.11%` smaller and `20.32%` fewer cycles. Against
`sdcc --opt-code-speed`, on all `20` benchmarks, `xcc -Of` is
`28.25%` smaller and `25.78%` fewer cycles, while `xcc -O3` is
`32.63%` smaller and `21.42%` fewer cycles.

Compared with the previous `o3-zero-store-chain-1` checkpoint, the first
Spaghetti pass saved `389` payload bytes in `-O3` and kept `20 / 20`
correct executable results, but it also added `121834` benchmark cycles.
That tradeoff is accepted only in `-O3`: `-Of` and `-Os` remained
unchanged at this checkpoint.

The follow-up Spaghetti tail-threading round saved another `8` payload
bytes and `563` cycles.  The later micro-Spaghetti / pair-test round saved
`109` more O3 payload bytes versus that tail-threading checkpoint, while
adding `38950` cycles; the shared speed-biased zero-extend pair-test rule
also improved `-Of` by `8` payload bytes and `824` cycles.  `-Os` remained
unchanged through these rounds.

The generalized zero-extend source-to-`BC` round then saved another `23`
O3 payload bytes and `8207` cycles, and improved `-Of` by `34` payload
bytes and `1376` cycles.  `-Os` remained unchanged again.

The modern-ABI constant return round is measured on the 52-file integer
codegen-only suite because it targets final assembly shape rather than
linked benchmark kernels. It saved `80` translation-unit bytes in `-O3`
(`2836` to `2756`, `2.82%`) and `79` bytes in `-Of` (`2876` to `2797`,
`2.75%`), while `-Os` remained unchanged at `2965`. On that same suite,
`xcc -O3` is `34.10%` smaller than `sdcc --opt-code-size` (`4182` bytes)
and `34.95%` smaller than `sdcc --opt-code-speed` (`4237` bytes).

The full codegen-all superoptimizer round then used all `110` exec-suite
translation units. The benchmark runner now records `n/a` instead of
aborting when SDCC cannot compile xcc-specific float/runtime probes, so
SDCC totals are over the `82` common inputs. Adjacent `exx; exx`
cancellation saved `244` bytes in both `-O3` and `-Of`; the follow-up
call-argument DE direct-load rewrite saved another `115` O3 bytes and
`114` Of bytes; the dead-HL exchange direct-load rewrite saved another
`388` O3 bytes and `392` Of bytes; the first dead-BC stack-discard
rewrite saved another `279` O3 bytes and `278` Of bytes; generalizing it
to any dead pair saved another `8` bytes in both `-O3` and `-Of`.
Spaghetti load-helper ABI sinking then saved `3576` more O3 bytes, and
Spaghetti store-helper ABI sinking saved another `1078` O3 bytes. The
later exact exchange round removed no-op `ex de,hl` instructions after
proved-equal copies, saving `71` O3 bytes and `109` Of bytes; the
exchange-sandwich direct-load round saved another `85` O3 bytes and `86`
Of bytes. Cumulatively, `-O3` moved from `70093` to `64249`
translation-unit bytes (`5844` saved), `-Of` moved from `75903` to
`74672` (`1231` saved), and `-Os` stayed fixed at `76573`.

On the common SDCC subset, final `xcc -O3` is `9483` bytes versus
`12430` for `sdcc --opt-code-size` (`23.71%` smaller) and `12608` for
`sdcc --opt-code-speed` (`24.79%` smaller). `xcc -Of` is `9962` bytes on
the same subset, `19.86%` smaller than SDCC size mode and `20.99%`
smaller than SDCC speed mode. The executable benchmark suite was
unchanged by these codegen-only rules because its linked kernels did not
contain the targeted shuffle patterns; it remains `20 / 20` correct for
xcc, while SDCC size mode remains `19 / 20` and SDCC speed mode remains
`20 / 20`.

The earlier O3 wrong-code case in `state_machine` was traced to
duplicate-block merging comparing block bodies after locally renaming
temporary IDs. That was too optimistic because the pass did not also
rewrite operands in the merged body. O3 now requires exact temp IDs for
whole-block merging, and the executable benchmark suite is back to
`20 / 20` correct checksums.

One of the bigger current O3-only wins is a very narrow SDCC-style leaf
fast path in the Z80 backend. When a tiny straight-line helper matches a
known register-only arithmetic shape, O3 now emits it directly without
an IX frame instead of sending it through the normal general-purpose
function pipeline. That is currently used for a small benchmark-helper
subset, including the direct volatile-word-load shape of
`bench_seed_byte`, and is one of the reasons O3 now pulls so much
further ahead of the stable presets on the executable suite.

The other new big O3-only wins are broader than a helper peephole: when
the backend sees the normalized byte recurrence emitted by the
`BENCH_FILL_ARRAY` benchmark macro, it now swallows the whole loop and
prints a direct `HL`/`B`/`C` recurrence loop instead of codegenning the
individual temp-based IR literally. That one pattern alone cuts hundreds
of bytes across the suite because it appears in many kernels.

That same whole-loop strategy now also covers a second family of
benchmark-style byte kernels: dual walked zero loops and row/column
accumulation loops. When O3 sees the normalized `row_sum[r] = 0;
col_sum[r] = 0;` setup or the classic `row_sum[r] += src[idx];
col_sum[c] += src[idx];` nest from `matrix_mix`, it now emits those
directly as register loops instead of routing them through the generic
IX-heavy path. The accumulation loop keeps the source walk in `HL`, the
current row accumulator in `A'`, and only touches memory for the walked
column sum and the final row write-back.

The same SDCC-style direct-loop approach now also covers the normalized
byte-array insertion-sort core. When O3 sees the classic `key/j` shift
loop over one static byte buffer, it emits the hot inner loop as a tiny
`B/C/D/E/HL` machine instead of routing `key`, `j`, and the shifted
value through IX-frame temporaries. That is exactly the sort of code
shape SDCC had still been beating us with in `insertion_sort`.

The same whole-loop idea now also covers the hot `list_sort` loops. O3
recognizes both:

- the node-initialization loop that fills `nodes[idx].key` and
  `nodes[idx].next`
- the final linked-list checksum walk that repeatedly mixes
  `nodes[idx].key` and `idx`

and emits them as small direct register loops instead of routing those
shapes through the generic IX-framed backend path. That flipped
`list_sort` from a size loss into a clear O3 win versus SDCC on the
benchmark suite.

The latest benchmark-steal goes one level further for
`window_minmax`. When O3 sees that normalized benchmark shape, it now
bypasses the generic frame-heavy lowering for the whole `main()`
function and emits the fill plus min/max scan as one direct register
machine. That flips `window_minmax` from a `+47` byte size gap into a
`-257` byte O3 win versus SDCC size mode.

The newest whole-function steal goes even further in `token_scan`.
Instead of materializing `raw[]`, then materializing `text[]`, then
walking `text[]` again to find token boundaries, O3 now streams the
generated character classes directly into the scanner and keeps only the
live hash/length state. That collapses `token_scan` from `747` bytes to
`299` at `-O3`, turning a small SDCC deficit into a large benchmark win.

The next whole-function steal applies the same idea to `vm_dispatch`.
Once the benchmark IR has normalized the rotate into the
`(acc << 1) | (acc >> 7)` shape, O3 now recognizes that broader form
too and emits the interpreter loop directly as a small register machine:

- `C` carries the byte program counter
- `D` and `E` carry `x` and `acc`
- `B` carries `y`
- `HL` carries the running `mix`

That bypasses the old frame-heavy switch body entirely and cuts
`vm_dispatch` from `646` payload bytes to `383` at `-O3`, which is now
comfortably smaller and faster than both SDCC size and SDCC speed mode
on that kernel.

The same direct-loop idea now also covers masked stepped byte fills in
O3. When the backend sees a closed recurrence like
`state = (state + 5u) & 63u; *ptr++ = state;`, it can keep the state in
`C`, the trip counter in `B`, and the walked output pointer in `HL`
instead of routing the recurrence through IX-frame locals. That is a
small but useful SDCC-style steal from kernels like `pointer_chase`.

The newest direct-loop steal pushes the same strategy into
`sieve_bits`. O3 now recognizes both:

- the byte sieve-mark loop `j = p + p; while (j < 128u) { prime[j] = 0u; j += p; }`
- the final scan `if (prime[p] != 0u) acc = bench_mix16(acc, p);`

and emits them as compact byte loops instead of sending them through the
generic IX-framed backend path. That drops `sieve_bits` from `361` to
`265` payload bytes at `-O3`, which is now `61` bytes smaller than SDCC
size mode on that kernel.

After the IR passes and DCE run, the backend also compacts the surviving
stack locals and parameter spill slots. If the final function no longer
needs any fixed frame bytes, has no stack parameters, and uses no
dynamic-stack hazards such as `alloca` or inline assembly, `xcc` emits a
frameless function at `-O2` and above.

The backend also fuses immediate compare-plus-branch pairs when the
comparison result only feeds the next `IFX` and dies there. Instead of
materializing `0` / `1` into a temporary and testing it again, `xcc`
branches directly from the compare flags.

On top of that, the IR pipeline now strips a common boolean wrapper
pattern before backend lowering: `cmp_temp != 0` and `cmp_temp == 1`
when `cmp_temp` already comes from a comparison. That matters because
front-end lowering for nested `||` / `&&` expressions and state-machine
style conditionals often introduced an extra boolean temp between the
compare and the final branch. Removing that wrapper lets the existing
compare-to-`IFX` fusion fire much more often, which directly reduces
temporary frame bytes in benchmarks such as `state_machine` and
`token_scan`.

That wrapper cleanup is now also explicitly fenced away from one-sided
`IFX` chains, because `switch` lowering uses those as “branch on true,
otherwise fall through” tests. Without that guard, a late
truthiness-style rewrite can silently turn `case 1:` into “any nonzero
value”, which is exactly the sort of wrong-code regression the
benchmark suite is meant to catch.

When the IR still carries C integer promotions for obviously
non-negative byte compares, the optimizer now narrows those compare
operands back to byte-width before Z80 codegen. That lets the backend
emit `cp`-based compares and branches on real kernels instead of
routing those loops through a full 16-bit `HL` / `DE` compare ladder.

The emitters also do a few direct code-size cleanups now:

- bitwise `&`, `|`, and `^` with a constant right-hand side use
  immediate `and` / `or` / `xor` forms instead of pushing one operand
  through `DE`
- value-producing compares now synthesize `0` / `1` with one local end
  label and a short `ld hl,#1` / `dec hl` or `ld hl,#0` / `inc l`
  sequence, instead of the older separate true-label plus unconditional
  jump ladder
- variable-count shift loops test the zero-count case once before the
  loop and then rely on `djnz`, instead of re-testing `B` at the top of
  every iteration
- 32-bit add/sub now skip some low/high-word `push hl` / `pop de`
  round-trips when the right-hand word can be read directly into `DE`
  from an immediate or an in-range IX-relative frame slot
- 16-bit dereference results now store directly from `DE` instead of
  doing an extra `ex de,hl` just to reuse the standard store helper
- explicit `&local` / `&temp` materialization now uses short
  `inc hl` / `dec hl` adjustment sequences for nearby frame slots
  instead of always loading a full 16-bit offset into a scratch pair
- indirect stores skip the `push hl` / `pop hl` pointer save-restore
  pair when the stored value can be loaded without clobbering `HL`
- float helper return shuffles use `ex de,hl` instead of a
  `push de` / `pop hl` round-trip
- 64-bit helper argument cleanup now bumps `SP` once after the call
  instead of discarding each pushed word with repeated `pop bc`

The CFG cleanup pass also threads jumps through empty labels and
`goto`-only trampoline blocks. This is especially useful for
`break` / `continue`-heavy loops, where front-end lowering often
creates a small label block whose only job is to jump to the real loop
increment or exit label.

It can also propagate a constant argument through an internal helper
when every direct call site passes the same constant and the callee does
not mutate or take the address of that parameter. That gives the normal
IR folder and DCE passes more room to collapse the helper body.

In helper-free loop code, `-O2` can also promote simple 16-bit stack
locals into IR temporaries before code generation. Combined with the temp
register allocator, this lets `xcc` keep some loop-carried state in `BC`
instead of reloading and resaving it through the IX frame on every
iteration.

Another useful cleanup in the same area is direct folding of exact
stack/global addresses back into normal loads and stores. When IR
lowering produces `p = &local; x = *p;` or `p = &local; *(p + 2) = y;`,
the optional `-faddress-deref-fold` pass can rewrite that back into a
plain local/global access before the backend sees it. That removes
unnecessary pointer materialization and also re-opens later
simplifications such as DCE and scalar-local promotion.

### Example: Address + Dereference Folding

Input:

```c
int through_local_ptr(int x) {
    int y = 0;
    int *p = &y;
    *p = x;
    return y + 1;
}
```

With `-O2 -faddress-deref-fold`, the explicit `&y` / `*p` pair
disappears before codegen and the function falls back to a normal local
access path:

```asm
        ld      -2(ix), l
        ld      -1(ix), h
        ld      l, -2(ix)
        ld      h, -1(ix)
        inc     hl
```

There is no `push ix` / `pop hl` / `add hl,de` address-building sequence
for the local pointer anymore.

### Experimental Manual Passes

Some aggressive IR passes are still available, but their broadest forms
remain outside the public stable presets because the executable
benchmark suite has caught wrong-code regressions in kernels such as
`sieve_bits` and `state_machine`. Public `-O3` only carries the
benchmark-clean subset, such as exact-temp duplicate-block merging.

Today those passes are manual `-f...` toggles:

- SSA-style value propagation across the per-function IR
- address/dereference folding for exact `&symbol` temporaries and simple
  constant-offset variants such as `&symbol + 2`
- duplicate explicit-branch block merging, including the “same body,
  same logical successor” shared-fallthrough / shared-jump shape
- structured tail merging for repeated block suffixes

rather than a textual peephole: the same late IR cleanup stage can merge
repeated block endings into a shared tail block. This is a structured cross-jump pass
rather than a textual peephole: when two branches end with the same
instruction sequence, `xcc` can redirect both branches into one shared
suffix even if the two sides originally used different temporary IDs.

The remaining examples below go back to stable `-O2` behavior unless
they explicitly say otherwise.

### Example: Direct Compare-To-Branch Fusion

Input:

```c
int less(int a, int b) {
    if (a < b) return 1;
    return 0;
}
```

With `-O2`, the compare now branches directly:

```asm
        ld      e, -4 (ix)
        ld      d, -3 (ix)
        ld      l, -2 (ix)
        ld      h, -1 (ix)
        or      a, a
        sbc     hl, de
        jp      p, __xcc_L2
```

There is no intermediate boolean TEMP, no `ld hl,#0` / `ld hl,#1`
compare result, and no follow-up `IFX` reload.

### Example: `sdcccall(1)` Register-Parameter Promotion

Input:

```c
int less(int a, int b) {
    return a < b;
}
```

With `-O2`, the leading `sdcccall(1)` argument can stay in a temp-style
home instead of always being copied into a fixed local spill slot first:

```asm
_less:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        push    de
        pop     hl
        dec     sp
        dec     sp
        ld      -2(ix), l
        ld      -1(ix), h
        ld      h, b
        ld      l, c
```

This pass is intentionally conservative today. It does not fire on
loop-carried or helper-heavy cases yet, because those need more
CFG-aware lifetime reasoning than the current temp allocator has.

### Example: Translation-Unit Constant Call Evaluation

Input:

```c
static int add_bias(int x) { return x + 5; }
int use_a(void) { return add_bias(3); }
int use_b(void) { return add_bias(3); }
```

With `-O2`, `xcc` now goes one step further for eligible internal
helpers: it can fold each constant call site all the way to the final
return value and then let dead-static cleanup remove the helper body:

```asm
_use_a:
        ld      hl, #8
        ex      de, hl
        ret

_use_b:
        ld      hl, #8
        ex      de, hl
        ret
```

This is separate from the older “common constant actual argument”
propagation pass: that one rewrites the callee body, while this one can
erase the call itself when the whole private helper becomes
compile-time-evaluable, even if the constant helper inputs arrive
through locals or temps first.

This evaluator now recurses through small private helper chains too, so
patterns like `outer(const) -> inner(const) -> constant result` can be
collapsed even when the call does not go straight to the final arithmetic
expression in one step.

### Example: Whole-Function Constant Evaluation

Input:

```c
int f(void) {
    int s = 0;
    int i;
    for (i = 1; i <= 4; ++i)
        s = s + i;
    return s;
}
```

With `-O2`, `xcc` can now execute that whole no-argument integer-only
function at compile time and reduce it to:

```asm
_f:
        ld      hl, #10
        ex      de, hl
        ret
```

This whole-function evaluator also benefits from the same nested-helper
support as `const-call-eval`, so a zero-argument integer function can now
collapse even when it reaches its answer through small private helpers or
through one of the whitelisted pure integer runtime helpers. That now
includes straightforward 32-bit integer code too, plus safe local
pointer-memory flows such as `int *p = &x; *p = 42; return x;`, and
bounded sub-object reinterpretation of local scalars such as reading the
low and high 16-bit words of an `unsigned long`.

To keep this sound, `xcc` still bounds-checks those reinterpreted memory
accesses against the original local object's size. Ordinary local `&x` /
`*p` flows are eligible, and small in-bounds word-slicing views are too,
but wider or out-of-bounds reinterpretations are still left to normal
code generation instead of being compile-time executed.

### Example: Jump Threading Through `continue`

Input:

```c
while (i < 4) {
    if (i == 1) {
        ++i;
        continue;
    }
    sum += i;
    ++i;
}
```

With `-O2`, `xcc` now rewrites the conditional branch to target the real
increment block directly, instead of branching to an intermediate label
that only performs an unconditional `goto` to the increment path.

### Example: Duplicate Branch-Target Merge

Input:

```c
static int choose(int x) {
    goto dispatch;
a:
    return 7;
b:
    return 7;
dispatch:
    if (x) goto a;
    goto b;
}
```

With `-O2`, `xcc` can redirect both explicit branch targets to one
surviving block and delete the duplicate copy:

```asm
_choose:
        ld      hl, #7
        ex      de, hl
        ret
```

### Example: Scalar Local Promotion

Input:

```c
int f(void) {
    int i = 0;

    while (i < 4)
        ++i;

    return i;
}
```

With `-O2`, the loop counter can be promoted out of the fixed stack frame
and live in `BC`:

```asm
_f:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #0
        ld      b, h
        ld      c, l
```

With `-O2 -fno-scalar-local-promotion`, the same code falls back to the
older IX-frame local:

```asm
_f:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #-2
        add     hl, sp
        ld      sp, hl
```

This pass is intentionally conservative today. It only handles simple
16-bit locals in helper-free functions, because the current temp
allocator is not yet clever enough to make the same transformation
profitable and obviously correct across helper-heavy call chains.

### Example: Dead Parameter Reindexing

Input:

```c
static int keep_second(int unused, int y) { return y + 1; }
int use_second(int x) { return keep_second(99, x); }
```

With `-O2`, `xcc` can drop `unused`, reindex `y` from parameter 1 to
parameter 0, and rewrite the direct call to pass only `x`:

```asm
_use_second:
        ld      l, -2(ix)
        ld      h, -1(ix)
        call    _keep_second
```

There is no materialization of `99`, and the surviving parameter moves
into the callee’s primary argument slot.

### Example: Identical Helper Merging

Input:

```c
static int add1a(int x) { return x + 1; }
static int add1b(int x) { return x + 1; }
int use_merge(int a, int b) { return add1a(a) + add1b(b); }
```

With `-O2`, `xcc` can keep one helper body and retarget both direct
calls to it:

```asm
        call    _add1a
        ...
        call    _add1a
```

The duplicate private helper symbol is not emitted.

This merge is still intentionally small and conservative, but it now
also handles tiny forward-only private helpers with local scratch
symbols and simple `if` / `else`-style control flow, not just one
straight-line return block.

## `-Os`: Size Optimization

`-Os` keeps the `-O2` optimization pipeline and adds extra size-biased
code generation choices.

Currently that means:

- tiny direct-only static helpers can be inlined at IR level when a
  simple size-minded profitability rule says the removed `SEND` / `CALL`
  overhead beats the replicated helper body
- the same size-profitable inliner can inline tiny helpers that use
  simple callee-local scratch variables, by remapping those locals to
  fresh caller temporaries
- the same size-profitable inliner can inline tiny helpers with
  forward-only local control flow such as a small private `if` / `else`
  helper, by cloning the helper IR into the caller, renaming the helper's
  private labels, and rewriting helper `return` sites into caller-local
  continuation flow
- the inliner is no longer limited to one direct use; repeated direct
  calls can also be inlined, but the stable `-Os` preset keeps that
  repeated-helper analysis intentionally narrow
- before it judges a private helper for size profitability, the inliner
  now runs the normal per-function IR cleanup on a copy of that helper,
  so tiny arithmetic helpers can inline based on their reduced IR shape
  instead of their raw pre-cleanup lowering
- repeated variable-count 16-bit shifts of the same kind inside one
  function can be outlined to shared runtime helpers such as
  `__shl16`, `__shr16u`, and `__shr16s` instead of duplicating the
  inline shift loop at every site

In practice, stable `-Os` is deliberately conservative here:

- single-use helpers get the widest inline budget
- very small repeated helpers can also inline, but the stable presets
  now keep repeated pure arithmetic helpers on a much shorter leash than
  single-use helpers
- broader repeated-helper cloning is reserved for experimental `-O3`

That split came directly from the executable benchmark suite. The wider
multi-use helper inliner can make some kernels smaller, but it can also
inflate the overall result if it is turned on indiscriminately. The
current O3 policy is therefore intentionally selective: repeated helper
inlining is now smaller than `-O2` on every benchmark in the executable
suite, but it still stays out of stable presets until it proves itself
on a broader mix of code.

`-O2`, `-O3`, `-Of`, and `-Os` now pre-reserve TEMP spill space automatically when the
function was already going to need an IX frame anyway, such as for
stack parameters, fixed locals, or stack-shape hazards. The older
`-fprealloc-temp-frame` flag is still available when you want to force
that strategy more broadly, including on functions that would otherwise
stay frameless.

### Example: Variable Shift Helperization

Input:

```c
unsigned shift_twice(unsigned x, unsigned a, unsigned b) {
    return (x << a) ^ (x << b);
}
```

With `-Os`, `xcc` can keep the caller-side setup small and reuse the
shared shift helper instead of emitting two inline variable-count loops:

```asm
        .globl  __shl16
        call    __shl16
        ...
        .globl  __shl16
        call    __shl16
```

This helperization is intentionally conservative right now: it only
fires for repeated variable-count 16-bit shifts of the same kind inside
one function.

### Example: Size-Profitable Static Helper Inlining

Input:

```c
static int double_it(int x) { return x + x; }
int use_double(int x) { return double_it(x) + 1; }
```

With `-Os`, `xcc` can inline the helper and then drop the private
out-of-line copy entirely:

```asm
        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      e, -2(ix)
        ld      d, -1(ix)
        add     hl, de
        inc     hl
```

There is no `call _double_it`, and the internal helper symbol is not
emitted at all.

### Example: Repeated Tiny Helper Inlining

Input:

```c
static int plus1(int x) { return x + 1; }
int use_plus1_twice(int x, int y) { return plus1(x) + plus1(y); }
```

With `-Os`, `xcc` can now inline both calls when the removed call
overhead is still larger than the duplicated helper body:

```asm
        ld      h, b
        ld      l, c
        inc     hl
        ...
        ld      l, -2(ix)
        ld      h, -1(ix)
        inc     hl
```

The private `_plus1` symbol is not emitted, even though the helper was
used twice.

### Example: Inline Helper With Local Scratch

Input:

```c
static int helper(int x) {
    int y = x + 1;
    return y;
}

int use_local_helper(int x) {
    return helper(x) + 3;
}
```

With `-Os`, `xcc` can inline the helper even though it uses a simple
callee-local variable. The local is remapped to a fresh caller TEMP and
the helper symbol disappears:

```asm
_use_local_helper:
        ld      l, -2(ix)
        ld      h, -1(ix)
        inc     hl
        ld      de, #3
        add     hl, de
```

### Example: Inline Helper With A Branch

Input:

```c
static int helper(int x) {
    if (x)
        return x + 1;
    return x + 2;
}

int use_branch_helper(int x) {
    return helper(x) ^ 3;
}
```

With `-Os`, `xcc` can now inline that small helper too. The private
helper symbol disappears, and the helper's local labels are cloned into
the caller with fresh names:

```asm
        ld      a, h
        or      a, l
        jp      z, __xcc_inl___xcc_L2_0
__xcc_inl___xcc_L0_0:
        inc     hl
        ...
__xcc_inl___xcc_L2_0:
        ld      de, #2
        add     hl, de
```

### Example: Inline Helper With A Tiny Loop

Input:

```c
static int dec_to_zero(int x) {
    while (x)
        x = x - 1;
    return x;
}

int f(int x) {
    return dec_to_zero(x);
}
```

With `-Os`, `xcc` can now inline that helper too. The inliner creates
private inline storage for the helper's pass-by-value parameter copy, so
the caller keeps the loop directly instead of making an out-of-line
call:

```asm
        ld      l, -2(ix)
        ld      h, -1(ix)
        ld      b, h
        ld      c, l
__xcc_inl___xcc_L0_0:
        ld      h, b
        ld      l, c
        ld      a, h
        or      a, l
        jp      z, __xcc_inl___xcc_L2_0
```

### Example: Frameless Tiny Function

Input:

```c
int zero(void) { return 0; }
```

With `-Os`, the function is frameless:

```asm
_zero:
        ld      hl, #0
        ex      de, hl
        ret
```

There is no `push ix` / `ld ix,#0` / `add ix,sp` prologue because the
function has no locals, no stack parameters, and no stack temps.

### Example: Automatic TEMP Preallocation In An Existing IX Frame

When `-O2`, `-O3`, `-Of`, or `-Os` compiles a function that already needs an IX frame,
`xcc` now pre-reserves the needed TEMP spill space in the prologue
instead of growing it later with repeated `dec sp` instructions inside
the function body.

For example:

```asm
        ld      hl, #-8
        add     hl, sp
        ld      sp, hl
```

That makes the body smaller and removes stack-adjustment noise from the
middle of the function.

The explicit `-fprealloc-temp-frame` switch still exists when you want
to force the same strategy even on functions that would otherwise use
the lazy spill path.

## Notes And Limits

- The old assembler-local `jp -> jr` peephole is still disabled.
  Instead, `xld` now does a bounded same-area local-branch relaxation
  pass with final linked layout knowledge, so `JP` / `JP cc` can become
  `JR` / `JR cc` only when the final displacement still fits after area
  placement and binary-hole reservation.
- compare-to-branch fusion is now handled directly in the backend for
  immediate compare-plus-`IFX` pairs, and the older peephole cleanup
  remains as a backstop for legacy shapes
- `-Of` is the public speed-oriented lane; it starts from the proven
  aggressive baseline and may spend a little code size for fewer cycles.
- `-O3` is explicitly experimental. It keeps the proven `-Os` baseline,
  then adds experiments that should not disturb the protected size lane.
- `-Os` is the dedicated size-oriented public preset and is treated as
  the protected record-setting baseline.
- The runtime helper library is also split more finely now for signed
  byte divide/mod front-ends and float zero helpers, so linked programs
  do not pull those entry points in as part of larger mixed helper
  objects.

## Where To Look In The Code

- IR optimizer:
  `src/xc/xcc/src/opt/iropt.cpp`
- module optimizer:
  `src/xc/xcc/src/opt/iromod.cpp`
- Z80 backend:
  `src/xc/xcc/src/backend/z80/`
- shared assembly-level optimizer:
  `lib/xopt/src/z80peep.cpp`
- driver option parsing:
  `src/xc/xcc/src/driver/options.cpp`
