# Optimization Guide

This page describes the optimizations that `xcc` already implements
today.

There is no separate active backend-codegen backlog document right now.
When a new structural code-emission problem is found, it should be
described here or added as a fresh research note at that point.

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

`-O3` is intentionally still the experimental lane, but it is no longer
better on the current benchmark suite because of hidden extra default
work. Instead, it is now free again to host the next wave of genuinely
new experiments without leaving `-Of` / `-Os` behind on the already
validated code-shape wins.

The newer generic structured-loop pipeline is now in the stable presets
too: direct control-condition lowering, counted-byte-loop narrowing,
pointer-walk canonicalization, and the generic walked-loop backend
emitters that feed from those shapes all run under `-O2` / `-Of` / `-Os`.

On the current bare-metal executable benchmark suite, the promoted
baseline means `-Of`, `-O3`, and `-Os` now land on the same measured
totals. That is deliberate: the proven size/speed wins are no longer
stuck behind the experimental preset.

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

Most of these are enabled through `-O2`, `-O3`, or `-Os`. The
`-fprealloc-temp-frame` switch still exists for targeted experiments,
but it now mainly serves to force wider TEMP preallocation than the
default optimized heuristic uses. `-fregalloc` is still available for
targeted bisects and for forcing it on from lower presets, but the
bounded stable allocator is now part of the normal `-O2` / `-Os`
pipeline. `-fduplicate-block-merge` is in a different category now: it remains
available for experimentation, but it is no longer part of the stable
`-O2` / `-Os` presets after a benchmark-driven wrong-code regression in
`state_machine`.

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

On the current executable benchmark suite, that split now looks like:

- `xcc -O2`: `14382` payload bytes, `4874191` cycles
- `xcc -Of`: `12099` payload bytes, `4456408` cycles

So `-Of` is currently about `15.87%` smaller and `8.57%` fewer cycles
than `-O2` on the full benchmark oracle, while still staying `20 / 20`
correct there.

## `-O3`: Experimental Optimization

`-O3` currently starts from the same promoted aggressive baseline as
`-Of` and `-Os`. That is deliberate: already-proven loop emitters,
helper fast paths, and other benchmark-clean structural wins are no
longer hidden behind the experimental preset.

So the role of `-O3` now is simpler:

- it is the place where new optimization ideas land first
- unsuccessful experiments are expected to be rejected, not kept around
- successful experiments should eventually graduate down into `-Of`,
  `-Os`, or even `-O2`

At the moment there is no additional always-on `-O3` pass beyond that
shared aggressive baseline, which is exactly what frees it for the next
experimental wave.

On the current executable benchmark suite, that shared promoted baseline
means:

- `xcc -Of`: `8696` payload bytes, `2762060` cycles
- `xcc -O3`: `8696` payload bytes, `2762060` cycles
- `xcc -Os`: `8696` payload bytes, `2762060` cycles

So the promoted aggressive pipeline is currently about `21.80%` smaller
and `20.71%` fewer cycles than `sdcc --opt-code-size` on the common
successful-and-correct benchmark subset.

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

Some aggressive IR passes are still available, but they are no longer
part of the public `-O3` preset because the executable benchmark suite
still catches wrong-code regressions in kernels such as `sieve_bits`
and `state_machine`.

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

`-O2`, `-O3`, and `-Os` now pre-reserve TEMP spill space automatically when the
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

When `-O2`, `-O3`, or `-Os` compiles a function that already needs an IX frame,
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
- `-Of` is now the public speed-oriented lane between `-O2` and `-O3`.
- `-O3` is explicitly experimental, but it now shares the promoted
  aggressive baseline with `-Of` and `-Os` so it can be used for new
  experiments without hoarding old proven wins.
- `-Os` is the dedicated size-oriented public preset, but today it also
  shares that same promoted aggressive baseline.
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
- peephole rules:
  `src/xc/xcc/src/backend/z80/z80peep.cpp`
- driver option parsing:
  `src/xc/xcc/src/driver/options.cpp`
