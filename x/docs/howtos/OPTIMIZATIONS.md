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
- `-Of`: validated speed optimization
- `-O3`: intentionally empty experimental alias of `-Of`
- `-Os`: size optimization

These are presets, not additive flags. You normally pick one of them.

`-O3` currently has exactly the `-Of` settings but retains a distinct profile
identity for the next speed experiment. `-Os` shares many validated
transformations but is the only profile that applies byte-count-only policy
where the backend has a choice.

`-Os` is deliberately conservative. New speed work is developed in the empty
`-O3` lane and promoted to `-Of` only after it is stable in both ABIs and on
independent corpora. A transformation also enters `-Os` only when its target
cost is Pareto-safe or it is selected by the size policy.

The newer generic structured-loop pipeline is now in the stable presets
too: direct control-condition lowering, counted-byte-loop narrowing,
pointer-walk canonicalization, and the generic walked-loop backend
emitters that feed from those shapes all run under `-O2` / `-Of` / `-Os`.

At this graduation point `-O3` intentionally contains no exclusive
transformation and produces the same output as `-Of`; subsequent speed
experiments may make it differ. Benchmark 23 omits the duplicate `-O3` rows
until an experiment makes that profile distinct again.

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
pipeline. `-fduplicate-block-merge` is part of the stable optimized
pipeline. Whole-block merging requires exact temporary operands, so two
similar tails that read different incoming temps are not redirected into
one shared block.

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
The same constant-call evaluator now also understands the pure fixed-point
runtime helper families for 8.8, 16.16, and 24.8 arithmetic, conversion,
comparison, negation, and absolute value, so fixed expressions such as
`fixed16_16_div(fixed16_16_from_int(3), fixed16_16_from_int(2))`
collapse to raw immediates before code generation.
In the `-O3`/`-Of` module pipeline, when only the fixed-point divisor is
constant, the module optimizer can also retarget exact positive integer
divisors `2`, `3`, `4`, and `8` to
small one-argument helpers such as `fixed16_16_div4`. That lets the
linker avoid pulling the full generic restoring divider for common
constant-divisor code while preserving signed truncation-toward-zero
semantics.
Constant fixed-point multipliers are handled the same way for common
fractions and ratios: `1/2`, `1/4`, `1/8`, `3/2`, and `5/4` become
tiny one-argument helpers. The 32-bit fixed formats use arithmetic-shift
multiply helpers for fractional multipliers so negative sub-unit values
match the generic multiply routines exactly.
The same evaluator recognizes those specialized helper names too, so a
private helper that was simplified to `fixed16_16_div4(x)` can still be
folded when its argument is constant. After constant-call folding, a
conservative single-assignment local propagation pass can replace stable
local integer constants across labels and loops, which lets code such as
`half = bench_frac(1, 2); ... fixed16_16_mul(x, half)` reach the
constant-multiplier retargeter.
Also in that experimental lane, calls to `fixed*_add` and `fixed*_sub`
are rewritten back to raw IR `ADD`/`SUB`, because those helpers are
defined as two's-complement wrapping integer arithmetic. This removes
call overhead and often avoids linking the add/sub helper archive
members for fixed-point-heavy loops.
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
- optional scalar local promotion for simple helper-free 16-bit stack locals
  whose address is never taken; this is excluded from public presets pending a
  proof-correct live-range interaction with register allocation
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

- guarded scalar-local promotion followed by physical Z80 register homes for
  eligible pointer, counter, and value live ranges; a structural dense-switch
  guard keeps selectors stack-homed when promotion would lengthen their live
  range
- adjacent, fully validated 32-bit conditional-shift/XOR recurrences keep the
  updated value in `DEHL` and commit it only after the final adjacent step;
  calls, aliases, volatility, escaping labels, and intervening IR are barriers
- the wider static-helper inline budget that used to live only in `-O3`
- dense switch jump-table lowering for integer switches when the backend
  can prove the span is profitable
- the same generic structured-loop pipeline that stable `-O2` already
  uses, but paired with the more aggressive speed-biased helper-inline
  thresholds
- speed-biased constant 16-bit shifts: `-Of` unrolls shifts by 6 and 7
  to avoid the `DJNZ` loop overhead, accepting a small size increase for
  fewer cycles
- two- and four-byte stack allocation with one or two `push af`
  instructions, which costs 11/22 cycles instead of 27 for the
  `ld hl,#-N; add hl,sp; ld sp,hl` sequence; five bytes and above retain
  the arithmetic form because the push sequence would be slower
- removal of a repeated register-pair immediate load across absolute stores
  when the intervening instructions prove that the pair is unchanged
- compaction of otherwise unused temporary frames through four bytes using
  the same speed threshold; the wider size-only threshold is not inherited
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

Performance numbers are kept out of this implementation guide. The audited
z88dk report records the current same-runner measurements, the overfitting
audit, independent holdouts, and validation totals.

## `-O3`: Experimental Speed Lane

`xcc -O3` currently is the validated `xcc -Of` pipeline with a distinct
profile identity and no exclusive transformation. It is empty so the next
experimental speed change has a clean measurement lane. Byte-count-only helper
sharing, tail merging, repeated-sequence outlining, and other
size-at-a-cycle-cost choices remain exclusive to `-Os`.

The former O3 whole-function selector was removed after the overfitting
audit. It contained large exact IR recipes developed against measured
programs, including benchmark-shaped loops and application-specific call
sequences. Matching a complete function by shape is benchmark recognition
even when the source filename is not inspected.

New speed work must first be expressed as a bounded transformation with an
explicit legality proof and target cost model, then validated on a frozen
corpus that was not used to design it. Only stabilized work is promoted from
`-O3` into `-Of`.

The speed pipeline's graduated legality guards include natural-loop
reaching-definition checks for pointer induction: a copied index is not
equivalent to the selected loop index when a competing inner-loop definition
can reach it through a backedge.
Late xopt rules likewise preserve register live-outs. For a
compiler-described temporary-frame slot, an IX-relative load followed by a
self-store may lose the store, but may lose the load only when every following
path overwrites `A` before reading it. Source-local slots are not rewritten
without volatility metadata.


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

Some aggressive IR passes remain outside the public presets because
their aliasing or control-flow proofs are not yet release-grade. `-O3`
does not enable them implicitly.

The remaining manual `-f...` experiments are:

- SSA-style value propagation across the per-function IR
- structured tail merging for repeated block suffixes

Address/dereference folding is stable in `-Of` / `-O3` / `-Os`.
Exact-temp duplicate-block merging is part of the optimized baseline.

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

With `-O2 -fscalar-local-promotion`, the loop counter can be promoted out of
the fixed stack frame and live in `BC`:

```asm
_f:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #0
        ld      b, h
        ld      c, l
```

With plain `-O2`, the same code stays in the older IX-frame local:

```asm
_f:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      hl, #-2
        add     hl, sp
        ld      sp, hl
```

This pass is experimental today and is not enabled by any `-O` preset. It
only handles simple 16-bit locals in helper-free functions, but a holdout
state-machine test exposed an unsafe live-range interaction across complex CFG
joins when register allocation followed it. Keep it opt-in until that proof is
complete.

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

- proof-bounded direct truncated-byte and masked-MSB shift/XOR lowering, plus
  the adjacent 32-bit recurrence spill sink shared with `-Of`; each of these
  is enabled here because it reduces bytes as well as cycles
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
- broader repeated-helper cloning remains outside the public presets

The wider multi-use helper inliner can make some programs smaller, but it can
also inflate the result if enabled indiscriminately. It remains outside the
public presets until it proves itself on a frozen, previously unseen mix of
code.

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
- `-O3` is the empty experimental alias of `-Of`; future experiments may spend
  code size for fewer cycles but must not inherit byte-count-only `-Os`
  choices.
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
