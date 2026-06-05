# XCC Z80 Optimization Proposal

## Goal

Improve generated code for an 8-bit Z80 target with a size-first bias.
The compiler should prefer:

- fewer bytes over desktop-style instruction count heuristics
- fixed, repeatable local rewrites over expensive global analysis
- small shared runtime helpers when they remove repeated inline
  sequences
- predictable code generation that the peephole pass can clean up well

This document is based on the current optimizer/backend structure in
`src/opt/iropt.cpp`, `src/backend/z80/z80gen.cpp`,
`src/backend/z80/z80gen_arith.cpp`, `src/backend/z80/z80peep.cpp`, and
`lib/runtime.s`.

## Current Baseline

The current pipeline is already headed in the right direction, but it is
still intentionally small.

- IR optimization is a fixed-point loop with constant folding, forward
  copy propagation, dead code elimination for pure temp-producing
  instructions, and a narrow power-of-two strength reduction.
- Code generation is HL/DE centric. Most 16-bit work is emitted
  directly, while multiply, divide, modulo, float, 64-bit arithmetic,
  TLS access, and indirect calls rely on runtime helpers.
- Temporary storage is still stack-heavy. Temps are lazily assigned
  IX-relative spill slots, and stack space for them is grown at first
  use by emitting `dec sp` or `ld hl,#-N / add hl,sp / ld sp,hl`.
- Register allocation is a lightweight pre-pass that can pin a 16-bit
  temp in `BC` or an 8-bit temp in `A'`, but only in narrow windows.
- The peephole pass is purely syntactic and already removes several
  common push/pop, self-load, and jump-to-next-label patterns.

That gives a good foundation, but several major sources of Z80 code
bloat remain:

- boolean values are often fully materialized as `0` or `1` and then
  immediately re-tested by `IFX`
- many simple 8-bit operations still travel through 16-bit paths
- stack cleanup after calls is often emitted with generic `add sp`
  sequences rather than the smallest target-specific idiom
- lazy temp allocation emits stack-adjustment noise inside the function
  body
- address computation and memory access patterns are not yet folded into
  Z80-shaped forms
- runtime helpers exist, but they are mostly arithmetic helpers rather
  than code-size helpers

## Guiding Principles

### 1. Optimize for code size first

On Z80, every extra spill, `push`/`pop`, label, or helper setup sequence
shows up immediately in output size. The first question for each
optimization should be "does this remove bytes from common code?"

### 2. Prefer target-shaped canonical forms

The backend and peephole pass should cooperate around a small set of
canonical patterns:

- `A` for byte values and tests
- `HL` for primary 16-bit values and pointers
- direct conditional jumps from flags whenever possible
- helper call ABI patterns that are easy to recognize and compress

### 3. Use runtime helpers deliberately

For Z80, a tiny shared helper is often better than re-emitting the same
8 to 20 byte sequence across many call sites. This is especially true
for:

- uncommon but repeated multi-byte operations
- pointer/addressing helpers
- sign/zero extension helpers
- variable-count shifts and structured compare helpers

### 4. Keep analysis cheap and local

A small C compiler for an 8-bit target benefits more from robust local
rewrites than from large SSA-only infrastructure. The best next steps
are still mostly basic-block or short-window transforms.

## Quick Wins

These should deliver useful savings quickly with limited architectural
risk.

| Layer | Proposal | Why It Helps On Z80 | Rough Approach | Difficulty |
| --- | --- | --- | --- | --- |
| IR | Add algebraic identities | Removes whole operations before backend emission | Fold `x+0`, `x-0`, `x*1`, `x/1`, `x%1`, `x&0`, `x&-1`, `x\|0`, `x^0`, shifts by `0`, `x-x`, double negation when safe | Low |
| IR | Fold compare-to-constant boolean cases | Prevents compare materialization into temp + later `IFX` re-test | Recognize `EQ/NE/LT/...` against `0`, `1`, and same operand pairs | Low |
| IR | Single-use temp forwarding | Removes stack temp churn that current copy-prop misses | If a pure instruction result is used once immediately, allow consumer-specific rematerialization or direct substitution | Low/Medium |
| Selector | Smaller call-argument cleanup | Current generic `ld hl,#n; add hl,sp; ld sp,hl` is expensive in bytes | Use `pop bc` for 2-byte cleanup, repeated `pop bc` for larger even counts, and `inc sp` only where needed | Low |
| Selector | Smaller helper-result moves | Several result shuffles can be expressed as `ex de,hl` or direct stores | Audit all `push`/`pop` result transfer idioms after helper calls and replace with direct register moves | Low |
| Selector | Byte-specialized arithmetic and logic | Many 8-bit values pay 16-bit costs today | Emit `inc a`, `dec a`, `add a,n`, `and n`, `or n`, `xor n`, `cp n` paths when type size is 1 | Medium |
| Selector | Direct branch from compare | Avoids building a 16-bit `0`/`1` temporary just to test it | Add compare-and-branch forms for `IFX` fed by `EQ/NE/LT/LE/GT/GE` temps | Medium |
| Peephole | Call cleanup compression rules | Backend currently emits a few large canonical cleanup sequences | Rewrite `ld hl,#2; add hl,sp; ld sp,hl` to `pop bc`; similar for 4/6/8 bytes | Low |
| Peephole | Zero/test/load simplification | `ld a,#0`, duplicate tests, and immediate reloads are common | Extend existing rules with `ld hl,#0` and compare/test simplifications | Low |
| Runtime | Tiny helper family for rare bulky idioms | Shared helpers can beat repeated inline setup | Consider helpers for variable 16-bit shift, compare-to-zero materialization, or repeated TLS access patterns | Low/Medium |

## Medium-Term Passes

These are the highest-value improvements after the quick wins.

### IR-Level Improvements

#### 1. Basic-block local value numbering

What it does:

- removes repeated pure expressions inside a block
- reuses already computed temps instead of regenerating loads and ops

Why it is effective:

- Z80 code bloats quickly when the same address calculation or compare is
  emitted twice
- local value numbering is much cheaper than full global CSE and fits
  the current non-SSA IR

Implementation approach:

- within a basic block, hash pure operations by opcode, operand ids, and
  type/size
- if a later instruction computes the same value and none of its inputs
  were invalidated, rewrite it to a copy
- run before DCE so duplicate computations disappear cleanly

Difficulty:

- Medium

#### 2. Address-expression folding

What it does:

- collapses `ADDRESS_OF + constant + GET_VALUE_AT/SET_VALUE_AT` into a
  single effective address

Why it is effective:

- the current backend often computes an address into `HL`, then performs
  another memory operation through it
- Z80 benefits from turning "address temp + dereference" into direct
  stack/global byte accesses whenever possible

Implementation approach:

- match patterns such as:
  `t1 = &obj`
  `t2 = t1 + k`
  `x = *t2`
- when the base is a local, parameter, or known global and the offset is
  constant, convert directly into an access of `obj+offset`

Difficulty:

- Medium

#### 3. Boolean canonicalization for branch consumers

What it does:

- keeps comparisons in flag-producing form when the only consumer is
  control flow

Why it is effective:

- the current `gen_compare` emits labels plus `ld hl,#0` or `ld hl,#1`,
  and `gen_ifx` then reloads and tests that result
- this is a classic source of unnecessary Z80 code size

Implementation approach:

- teach IR lowering or a late IR pass to distinguish:
  "value compare needed" from "branch decision only"
- represent branch-only compares directly, or fuse
  `CMP_TEMP -> IFX(CMP_TEMP)` before backend emission

Difficulty:

- Medium

#### 4. Store-to-load forwarding for temps and locals

What it does:

- eliminates immediate reloads from the same stack slot or temp location

Why it is effective:

- current peephole catches one narrow temp store/reload pattern, but the
  backend still produces many short-range stack round trips

Implementation approach:

- in a block, remember the last stored value for a stack slot or temp
- if the next load is from the same location with no intervening kill,
  reuse the value or rewrite the load away

Difficulty:

- Medium

### Instruction Selection And Codegen

#### 5. Pre-allocate spill area once per function

What it does:

- computes required temp spill space ahead of time and reserves it in the
  prologue instead of emitting stack growth at first temp use

Why it is effective:

- this is likely one of the biggest code-size wins available
- current lazy `alloc_temp()` emits `dec sp` noise throughout the
  function body
- fixed spill offsets also make later peephole and addressing logic much
  simpler

Implementation approach:

- perform a pre-pass to assign all non-register temps fixed frame
  offsets
- compute `max_temp_bytes`
- reserve `local_bytes + max_temp_bytes` in the prologue once
- stop emitting stack adjustment instructions from `alloc_temp()`

Difficulty:

- Medium

#### 6. True 8-bit operation paths

What it does:

- lowers byte-typed operations directly through `A` and byte registers
  instead of promoting everything to 16-bit HL/DE flows

Why it is effective:

- the ABI may still return bytes in `L`, but computation does not need
  to keep zero-extending through `HL`
- this saves loads, stores, high-byte clears, and temporary stack usage

Implementation approach:

- add 8-bit variants for `ADD`, `SUB`, `NEG`, `BAND`, `BOR`, `BXOR`,
  `BNOT`, shifts, compares, and casts
- allow 8-bit temps to stay in `A` or memory without forcing `HL`
  materialization

Difficulty:

- Medium

#### 7. Compare/branch fusion in the backend

What it does:

- emits a direct compare followed by `jp cc,label` when the compare
  result is only used by `IFX`

Why it is effective:

- removes labels, result materialization, reloads, and second-stage
  flag tests
- aligns with the Z80’s real strength: branch immediately off flags

Implementation approach:

- inspect the use of compare-result temps during backend emission or in a
  late lowering pass
- introduce `gen_if_compare()` style emission for fused patterns

Difficulty:

- Medium

#### 8. Better small-constant instruction selection

What it does:

- picks byte-cheaper forms for common small constants and masks

Why it is effective:

- Z80 has many special cases worth exploiting:
  `xor a` for zero, `inc`/`dec`, `or a` for test, and direct byte masks

Implementation approach:

- extend current `±1` handling to include:
  `& 0x00`, `& 0xFF`, `| 0`, `^ 0`, add/sub small byte constants,
  shift-by-8 special cases for more ops, and cast-driven zero/sign
  extension shortcuts

Difficulty:

- Low/Medium

#### 9. Better stack-argument emission

What it does:

- reduces argument push overhead, especially for constants, bytes, and
  helper calls

Why it is effective:

- function-call-heavy code pays this cost constantly
- today 8-bit args are zero-extended into `HL` before push

Implementation approach:

- consider a dedicated byte-push helper sequence for frequent byte
  arguments
- when a helper ABI is internal to xcc, allow more compact conventions
  than the full general C-call sequence
- special-case literal zero and small immediates

Difficulty:

- Medium

### Peephole Improvements

#### 10. Canonical stack-adjustment rules

What it does:

- rewrites generic SP arithmetic into the smallest correct idiom

Why it is effective:

- this targets a pattern the current backend emits often enough to matter

Implementation approach:

- add rules for:
  `ld hl,#2; add hl,sp; ld sp,hl -> pop bc`
  `ld hl,#4; add hl,sp; ld sp,hl -> pop bc; pop bc`
  `ld hl,#6; add hl,sp; ld sp,hl -> pop bc; pop bc; pop bc`
- keep them limited to windows where `BC` is dead or declared scratch by
  calling convention

Difficulty:

- Low

#### 11. Load/store cancellation and forwarding

What it does:

- expands the existing temp-store-reload rule into a more general family

Why it is effective:

- stack slot traffic is one of the compiler’s biggest visible costs

Implementation approach:

- match short windows such as:
  `ld X,a; ld a,X`
  `ld X,l; ld Y,h; ld l,X; ld h,Y`
  `push rr; pop rr2` transformations
- optionally track a tiny abstract register/value state inside one basic
  block of assembly lines

Difficulty:

- Medium

#### 12. Branch layout cleanup

What it does:

- removes tiny branch ladders and label pairs left by compare lowering

Why it is effective:

- the current compare emission creates labels and unconditional jumps
  even for simple boolean cases

Implementation approach:

- teach the peephole pass to collapse:
  `jp cc,L1; ld hl,#0; jp L2; L1: ld hl,#1; L2:`
  when later usage shows only a branch or a byte-sized boolean is needed
- this may also be better solved earlier, but a peephole backstop is
  valuable

Difficulty:

- Medium

### Runtime-Helper Opportunities

#### 13. Code-size helpers for variable shifts

What it does:

- moves variable-count shift loops out of line

Why it is effective:

- current variable shifts inline a loop body, labels, and count setup
  each time
- on an 8-bit target, a shared helper can be a net win if shifts are not
  extremely hot

Implementation approach:

- add helpers such as `__shl16`, `__shr16u`, `__shr16s`
- keep a threshold:
  small constant counts stay inline, variable or large counts call the
  helper

Difficulty:

- Medium

#### 14. Compare helpers for infrequent wide operations

What it does:

- centralizes bulky 32-bit and 64-bit compare sequences

Why it is effective:

- wide compares are byte-expensive, especially when followed by boolean
  materialization
- a helper can return flags or a compact relation code

Implementation approach:

- add optional helper families for 32-bit and 64-bit compare/test
- use only when the inlined sequence would exceed a size threshold

Difficulty:

- Medium

#### 15. TLS access helpers

What it does:

- reduces repeated `__tls_base + add offset + load/store` sequences

Why it is effective:

- current TLS access emits a multi-instruction sequence on every load and
  store
- TLS-heavy code will grow quickly

Implementation approach:

- for byte/word accesses, consider helpers like:
  `__tls_load_1(offset)`, `__tls_load_2(offset)`,
  `__tls_store_1(offset,val)`, `__tls_store_2(offset,val)`
- alternatively cache the TLS base within a function when safe

Difficulty:

- Medium

## Advanced Ideas

These are worthwhile, but they should follow the earlier work.

### 1. EXX-block allocation

What it does:

- uses alternate register set windows (`EXX`, and selectively `AF'`) for
  short expression regions

Why it is effective:

- the current allocator explicitly avoids `BC'/DE'/HL'`
- a controlled EXX region could cut stack traffic substantially in
  arithmetic-heavy code

Implementation approach:

- define backend regions where alternate registers are reserved for the
  expression evaluator
- ensure calls, shifts, helper setup, and memory paths do not cross the
  region unsafely

Difficulty:

- High

### 2. Superoptimizer-fed peephole rule generation

What it does:

- builds a library of verified short Z80 replacements for small code
  windows

Why it is effective:

- Z80 has many non-obvious byte-saving idioms that humans discover over
  time
- a generated rule set can outperform ad hoc manual peepholes

Implementation approach:

- enumerate short instruction windows for selected semantics
- validate replacements with an emulator or symbolic checker
- import only small, robust, high-hit-rate rules

Difficulty:

- High

### 3. Profile-aware inline-vs-helper selection

What it does:

- chooses between inline code and helper calls based on size policy and
  measured hotness

Why it is effective:

- some operations should stay inline in tight loops but move out of line
  elsewhere

Implementation approach:

- start with static heuristics, then optionally add profile feedback
- make helper use an explicit optimization policy rather than a fixed
  rule

Difficulty:

- High

### 4. SSA-style mid-end for larger transformations

What it does:

- enables better PRE, global CSE, and more precise value tracking

Why it is effective:

- it would improve more than just the Z80 backend

Why it is not first:

- it is a major architectural step and does not match the current
  codebase’s "quick local gain" opportunity profile

Difficulty:

- Very High

## Recommended Implementation Order

### Phase 1: Immediate Size Wins

- add algebraic identity folding in `iropt`
- fuse compare-to-branch for `IFX`
- replace generic call stack cleanup with compact Z80-specific forms
- audit helper-result transfer sequences and remove redundant push/pop
- add a few more peephole rules around stack cleanup and zeroing

Expected result:

- noticeably smaller code without redesigning the compiler

### Phase 2: Reduce Stack Traffic

- pre-allocate temp spill space in the prologue
- add single-use temp forwarding and store/load forwarding
- improve 8-bit operation selection
- fold address expressions into direct accesses

Expected result:

- lower spill noise, fewer IX-relative reloads, and better code for
  ordinary scalar C

### Phase 3: Runtime-Helper Strategy

- add out-of-line helpers where repeated inline loops are large
- focus first on variable shifts and TLS-heavy access patterns
- add compare helpers only when size measurements justify them

Expected result:

- smaller binaries for codebases that use these operations repeatedly

### Phase 4: Advanced Backend Work

- explore EXX-region allocation
- build a larger verified peephole rule set
- consider broader IR modernization only after the earlier passes settle

## Highest-Value Recommendations

If only a few items are implemented first, the best bets are:

1. Pre-allocate temp spill space once per function.
2. Fuse compare generation with `IFX` so boolean temps are not
   materialized unless truly needed.
3. Replace generic post-call stack cleanup with `pop`-based Z80 forms.
4. Add true 8-bit operation lowering instead of routing byte values
   through 16-bit paths.
5. Expand IR algebraic simplification and direct address folding.

Those five changes fit the current architecture, target the biggest
repeated size costs in the existing code generator, and do not require a
desktop-style optimizer redesign.

## Success Metrics

Measure progress with Z80-specific output metrics, not just generic
compiler metrics.

- total text size across a fixed benchmark corpus
- average bytes per C statement for small integer code
- number of emitted `push`/`pop` pairs per function
- number of emitted IX-relative temp loads/stores per function
- number of compare-result temps that survive to assembly
- runtime helper call count by helper kind

The best next step is to establish a small benchmark suite and track
assembly byte deltas per optimization phase.
