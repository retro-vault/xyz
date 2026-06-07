# SDCC Z80 Codegen Notes

This note compares `xcc` with the original SDCC Z80 backend under
`orig/sdcc/sdcc/` and captures the parts of SDCC's code-generation
strategy that are most relevant to closing the benchmark gap.

It is intentionally practical. The goal is not to clone SDCC blindly,
but to identify the mechanisms that make its emitted code smaller and
faster on Z80 and then adapt those ideas in `xcc`.

## Source Map

The main SDCC files worth studying are:

- `orig/sdcc/sdcc/src/z80/ralloc.c`
- `orig/sdcc/sdcc/src/z80/ralloc2.cc`
- `orig/sdcc/sdcc/src/z80/gen.c`
- `orig/sdcc/sdcc/src/z80/peep.c`
- `orig/sdcc/sdcc/device/lib/z80/README`

## What SDCC Does Better

### 1. It treats register allocation as a first-class backend problem

SDCC still supports an older allocator in `ralloc.c`, but it also ships
the newer `ralloc2.cc`, which is a proper cost-driven allocator.

The important takeaways are:

- SDCC is not trying to keep every local in memory and only sprinkle a
  few temps into registers.
- It explicitly scores assignments by where bytes land: `A`, register
  pairs, partial spills, and rematerialization all have different cost.
- It gives bonuses for sane pair placement such as `BC`, `DE`, and `HL`
  and penalties for awkward split placement.

That matches what our benchmarks are telling us: the biggest gap is not
syntax-level cleanup, it is the volume of frame traffic we generate
before peephole ever runs.

### 2. It keeps state about what register pairs already hold

In `gen.c`, SDCC keeps persistent state for register pairs and stack
offsets:

- what `HL`, `DE`, `BC`, `IX`, etc. currently contain
- current stack depth and parameter offsets
- preserved and temporary register state

That lets SDCC avoid many reloads and often adjust an already-live pair
instead of rebuilding it from scratch.

This is a major contrast with `xcc`, where we still tend to emit:

- repeated `ld l, N(ix)` / `ld h, N+1(ix)`
- repeated frame reloads for the same local or temp
- stack save/restore scaffolding around short-lived pair use

### 3. Its peephole pass is dataflow-aware

`orig/sdcc/sdcc/src/z80/peep.c` is not just a fixed-window text
rewriter.

It tracks things such as:

- whether a label is a real jump target
- which registers are read or preserved across calls
- whether a condition actually consumes a given flag
- whether a candidate rewrite is safe in the surrounding control flow

Our `z80peep.cpp` has grown a useful rule set, but it is still mostly
syntactic. SDCC's peephole is much better at proving a rewrite is safe
across a slightly wider window.

### 4. It optimizes around real Z80 cost, not generic IR taste

The Z80 README in `device/lib/z80/README` is blunt about one of the
central machine facts:

- `ld r,(ix+n)` is expensive
- `ld r,(hl)` plus `inc hl` is often cheaper

That mindset shows up all through SDCC:

- avoid IX when a live pointer walk will do
- prefer short branches when possible
- prefer pair reuse over frame reloads
- keep runtime helpers small and register-friendly

### 5. It uses short branches heavily

On our benchmark suite, SDCC emits large numbers of `jr` where `xcc`
historically emitted only `jp`.

That matters twice:

- one byte smaller per shortened branch
- usually faster control flow too

This is one of the easiest structural differences to see in the raw
assembly output.

## What `xcc` Historically Did Instead

Before the recent benchmark-driven work, `xcc` tended to:

- spill incoming register arguments into the IX frame immediately
- keep loop-carried state in frame slots
- materialize boolean control flow through longer branch ladders
- prefer `jp` everywhere
- rely on peephole cleanup only after much larger frame-based code had
  already been emitted

The result was exactly what the benchmark assembly showed:

- far more `ix` references than SDCC
- far more `push` / `pop`
- essentially no `jr`
- much larger hot loops

## Adaptation Plan

These are the main SDCC-inspired adaptations we should keep driving:

- keep incoming register arguments live longer
- shrink local control flow to `jr` where safe
- reuse already-live pairs instead of rebuilding them from frame slots
- promote loop-carried locals out of the IX frame
- make more of the late peephole/dataflow logic aware of actual register
  and flag liveness

## Landed So Far

### Conservative incoming-argument retention

`xcc` now keeps some leading `sdcccall(1)` register arguments live
through their first real use instead of always spilling them in the
prologue. If they must live longer, they are materialized to the frame
later.

That is a small step toward SDCC's more register-centric style.

### Conservative short-branch relaxation

`xcc` now shortens eligible `jp` branches to `jr` in the late Z80
peephole pass, using a conservative section-local byte layout estimate
instead of the older source-line heuristic.

This is not a full assembler- or linker-level relaxer, but it moves us
closer to SDCC's much heavier use of relative control flow without
breaking stable builds.

### Conservative pair-content tracking

`xcc` now keeps a small exact-content cache for `HL` and `DE` under
stable optimized builds. When straight-line code asks to reload the same
local, temp, or immediate into one of those pairs, the backend can now
skip the duplicate reload instead of rebuilding it from the IX frame.

This is intentionally much narrower than SDCC's richer pair-state model:

- labels flush the cache, so joins do not inherit stale pair contents
- calls flush the cache
- bytewise pair mutations such as `srl h` / `rr l` flush the affected
  pair
- memory stores conservatively flush the cache before it is reseeded
  with a known post-store value

That narrow policy is what keeps the optimization stable enough for the
default `-O2` / `-Os` presets.

### Conservative `A`-content tracking

`xcc` now also keeps a narrow exact-content cache for `A` under the
stable optimized presets. When straight-line code reloads the same
local byte, temp byte, or constant into `A`, the backend can now skip
that duplicate load if no proven `A` clobber happened in between.

This is intentionally even narrower than the pair cache:

- labels and calls flush it
- `ex af,af'` flushes it
- byte arithmetic/rotate instructions that write `A` flush it
- volatile/global/TLS byte reads do not seed it

This is still far smaller than SDCC's richer backend state tracking, but
it follows the same core idea: do not repay an IX-frame load when the
backend already knows exactly what is still in a register.

### Bounded stable temp allocation in `BC`

`xcc` now also ships a bounded register-allocation prepass in the stable
`-O2` / `-Os` presets. It is deliberately much smaller than SDCC's real
allocator:

- it only considers 16-bit temps
- it only uses the main `BC` pair
- it only accepts short, straight-line, gap-free live windows
- it rejects intervals that cross known backend scratch hazards

That still turned out to matter a lot on the executable benchmark suite:
it removed a large amount of IX-frame churn without reopening the
wrong-code failures that the earlier, looser experiments had.

### Scalar-helper TEMP slot reuse

`xcc` now also reuses stack TEMP spill slots by live range for one very
small function class: straight-line scalar helpers that do not touch
nonlocal storage, do not make calls, and do not go through pointer-style
memory operations.

This is not yet SDCC's broader allocator story. It does not try to solve
general slot reuse across arbitrary CFGs or memory-heavy code. But it does
capture one useful SDCC-style principle: a helper frame should be sized by
how many temporaries are simultaneously live, not by the sum of every temp
the frontend ever named in that function.

### Real byte-sized TEMP spill slots

`xcc` now also sizes stack-resident TEMP slots by their real byte width
instead of silently rounding every spilled anonymous TEMP up to a full
word in the frame planner. On Z80 that matters a lot in byte-heavy loops:
the old planner could easily waste dozens of frame bytes and a large
amount of IX-relative traffic just because most intermediate values came
from `unsigned char` arithmetic.

This is still not a substitute for SDCC's broader register allocator, but
it is exactly the kind of backend honesty SDCC gets right: if a temporary
is one byte wide, the frame should treat it as one byte wide unless there
is a concrete reason not to.

### Cheap address-temp rematerialization

`xcc` now also rematerializes one very specific class of 16-bit temps
instead of always spilling them to the IX frame:

- `ADDRESS_OF` temps
- `base + u8-index` temps where the base is a direct object address
- the same shape when the base came from a preceding `ADDRESS_OF` temp

The important fix here was not just the codegen hook. The temp
live-interval builder had to learn that `SET_VALUE_AT` uses its pointer
through the `result` operand slot rather than the usual `left`/`right`
positions. Until that was fixed, these address temps looked dead too
early and never qualified for rematerialization at all.

### Byte-compare narrowing before backend lowering

`xcc` now also strips one common benchmark anti-pattern before Z80
codegen: widened byte `EQ` / `NE` comparisons that were only using a
16-bit temp because the frontend had promoted the byte earlier.

When both sides really still come from byte-sized values, the IR pass
collapses the compare back to byte operands so the backend can emit a
direct `cp` instead of sign-extending into a full `sbc hl,de` compare
sequence.

This is a very SDCC-like improvement in spirit: the machine code should
reflect the real width of the comparison, not the most conservative
intermediate width the frontend happened to produce.

### Late nonzero-bool materialization cleanup

`xcc`'s late peephole pass now also removes one of the remaining generic
boolean scaffolds SDCC avoids: full 16-bit `HL != 0` materialization
blocks when the value being tested is already live in `HL`.

That matters because branch-heavy kernels were still producing dozens of
those sequences even after earlier compare cleanup. Collapsing them back
into a direct flag test is exactly the kind of late structural cleanup
that SDCC's backend tends to do more aggressively than ours.

That is a good example of the SDCC lesson in miniature: once the
backend knows a value is cheap to rebuild, it should stop paying both
frame bytes and reload traffic for it.

### Very narrow stable `A'` allocation for byte chains

`xcc` now also uses a tiny, benchmark-safe subset of the old alternate-A
idea. When an 8-bit TEMP is defined and its only use is in the very next
instruction, the stable allocator can keep it in `A'` instead of spilling
it to the IX frame.

That is intentionally much narrower than a general byte-temp allocator:

- only next-use-only byte temps qualify
- control-flow barriers still reject the window
- compare/flag-sensitive windows are still rejected

Even with those restrictions, it trims a lot of waste from byte-heavy
chains because many Z80 byte expressions really do lower as “produce one
intermediate, consume it immediately, produce the next one”.

### Narrowed promoted byte compares

`xcc` now also recognizes one very common promotion shape that was
blocking smaller Z80 code: compares where both sides started as
unsigned bytes, were widened to `int` only because of the usual integer
promotions, and then were compared immediately.

When the IR can prove both sides are still in the `0..255` range, it
rewrites the compare operands back to byte-width before Z80 codegen.
That lets the backend emit `cp`-based branches and boolean compares
instead of a full 16-bit `HL` / `DE` compare ladder.

This is a useful reminder from SDCC's style of backend work: part of
the size win comes from preserving machine-relevant narrow integer facts
long enough for the backend to exploit them.

### Compare-bool chain cleanup

Another SDCC-style lesson is that front-end boolean lowering matters
just as much as late peepholes. `xcc` now strips one very common wrapper
shape before Z80 codegen:

- `cmp_temp != 0`
- `cmp_temp == 1`

when `cmp_temp` already comes from a comparison. Those wrappers used to
block the existing compare-to-branch fusion, especially in nested
short-circuit code and state-machine kernels. Cleaning them up earlier
lets the backend branch directly from the original compare more often,
which cut a noticeable amount of TEMP frame and push/pop traffic from
`state_machine`, `token_scan`, and similar control-heavy benchmarks.

### Direct control-condition lowering

`xcc` now also lowers statement conditions more like SDCC does. For
`if`, `while`, `do ... while`, and `for`, short-circuit `&&`, `||`, and
`!` are now emitted as branchy IR directly instead of first building a
larger value-style boolean tree and then trying to recover the branch
shape later.

That matters because SDCC's backend wins are not only about register
allocation. They also come from never inventing large boolean TEMP
forests in the first place for control-heavy code.

### One-sided IFX guarding for switch chains

The benchmark suite also flushed out one subtle trap in this area.
`switch` lowering uses one-sided `IFX` nodes to mean “branch on true,
otherwise fall through”. That means late truthiness-style simplifiers
must not rewrite `x == 1` into just `x`, or invert `x == 0` into a raw
false-branch, because that silently changes case dispatch semantics.

`xcc` now explicitly fences those one-sided `IFX` forms off from the
late compare-wrapper cleanup passes. That guard is a good example of the
kind of backend discipline SDCC's mature lowering has baked in already:
size wins are only real wins if they preserve the odd little control
forms that the frontend relies on.

### Narrow BC residency for direct local / parameter state

`xcc` now also has a very small symbol-backed extension of the stable BC
allocator. In addition to short-lived 16-bit temps, the backend can now
keep some simple 16-bit locals and parameters in `BC` when they stay
inside a tiny straight-line window with no address-taking and no obvious
BC scratch hazards.

This is still far from SDCC's real allocator. It does not yet do global
pair placement or broader live-range splitting. But it is an explicit
step toward the SDCC idea that non-temporary program state should be
eligible for register residency too.

### Direct constant-address memory access

`xcc` now also has a direct absolute-memory path for dereferences of
literal pointer addresses. When the address is already exact, the
backend no longer needs to spend `HL` just to reconstruct it before a
byte or word load/store.

That is a small fix in absolute terms, but it matches an important SDCC
habit: do not waste scarce registers on addresses the compiler already
knows exactly.

### Rotate idiom recognition and smaller constant shifts

`xcc` now also recognizes one SDCC-style size opportunity that was
showing up all over the benchmark helpers: unsigned 16-bit rotate idioms
written in ordinary C as `(x << k) | (x >> (16 - k))`.

Instead of lowering those as two separate shifts plus an OR, the IR now
combines them into a rotate operation and the Z80 backend emits a much
smaller rotate-style sequence.

At the same time, constant 16-bit shifts are now emitted more like the
compact SDCC style:

- small constant shifts are fully unrolled
- larger constant shifts no longer pay a redundant zero-count branch

This does not solve the whole benchmark gap, but it is a good example of
the kind of machine-aware pattern reduction SDCC keeps applying.

### Byte-native lowering and deeper promoted-byte narrowing

One of the most expensive differences in the benchmark suite was that
`xcc` still routed many byte expressions through 16-bit `HL` code and
only truncated them back to a byte afterward.

That has now improved in two layers:

- the IR `promoted-byte-ops` pass can follow a longer single-use chain
  where byte values were widened through `unsigned int`, used in a small
  arithmetic chain, and then cast back down
- the Z80 backend now emits byte-native code for byte-sized
  `ADD`/`SUB`, `AND`/`OR`/`XOR`, `BNOT`, and byte shifts

This is much closer to the way SDCC treats byte-heavy kernels like
`gray_decode`, `histogram`, and `fir_shiftadd`: keep the hot state in
byte operations for as long as possible and only widen when the machine
actually needs it.

### Folding `&global` temporaries back into direct addresses

Another SDCC-style win now landed in `xcc` is that temporary results of
`&global_object` no longer have to live in stack slots just to feed
later `base + index` arithmetic.

Stable optimized builds now rewrite those temp uses back into direct
label-address operands. That lets the backend emit immediate-address
loads for the base and leaves dead-code elimination free to remove the
old address temp materialization.

### Pre-cleaned repeated-helper inlining

`xcc` now analyzes a cleaned-up copy of a private helper before deciding
whether it is small enough to inline repeatedly. That matters because the
raw lowered IR for helpers like `bench_mix16` is noticeably larger before
the normal per-function simplifier runs, even though the final reduced
shape is exactly the kind of tiny arithmetic leaf that SDCC tends to
inline away.

The practical result is:

- stable `-O2` can now inline repeated tiny pure helpers such as
  `bench_mix16`
- `-O3` still keeps the broader analysis budget for larger multi-use
  helpers
- `-O3` now also carries a slightly wider repeated-helper cloning gate
  that is large enough to inline the remaining benchmark `bench_seed_byte`
  call sites, while still staying outside the stable presets
- `-O3` also now carries an experimental late switch-jump-table pass
  that recognizes dense compare-ladder dispatch and rewrites it to the
  same basic indexed `jp (hl)` shape SDCC emits for kernels such as
  `state_machine`, `token_scan`, and `vm_dispatch`

This is a good example of an SDCC-style lesson that is not a peephole at
all: do not judge profitability from the raw first-lowered form when you
already know the backend will simplify that helper substantially before
emission.

## Next High-Value Steps

The next most promising adaptations are:

1. Expand the bounded allocator beyond short temp windows so more
   loop-carried scalars stop round-tripping through IX.
2. Teach the peephole pass a small amount of register/flag liveness so
   it can safely remove wider save-restore and reload patterns.
3. Rework loop-heavy local array code so `HL` walks memory directly more
   often instead of repeatedly re-deriving addresses from `IX`.

## Rule Of Thumb

SDCC is not winning because of one magic peephole. It wins because its
backend does three things together:

- it assigns more values to useful registers
- it remembers more about what those registers already contain
- it spends fewer bytes on frame management and long control flow

That is the direction `xcc` needs to keep moving.
