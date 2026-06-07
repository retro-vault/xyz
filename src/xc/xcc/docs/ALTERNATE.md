# Alternate Register Proposal for the Z80 Backend

## Purpose

This document proposes a practical way to exploit the Z80 alternate
register set better in `xcc` without turning the backend into a hard to
debug register allocator experiment.

The goal is not "use every alternate register everywhere".
The goal is:

- get clear wins where the hardware genuinely helps
- preserve the current readable backend structure
- make clobbers and ABI constraints explicit
- avoid subtle flag and `EXX` bugs

## Current State

Today the backend already does a small amount of alternate-register use:

- `src/backend/z80/z80gen_regalloc.cpp` assigns one 16-bit temp to main
  `BC` when its live range stays inside a short straight-line window and
  does not cross a known `BC` scratch hazard.
- The older `A'` byte-temp experiment is still implemented in pieces of
  the backend, but it is not part of the stable preset allocator today.
- `src/backend/z80/z80gen.cpp` implements those assignments in
  `emit_load_rr()`, `emit_store_rr()`, `load_a()`, and `store_a()`.
- The backend does not currently use `BC'`, `DE'`, or `HL'` in general
  code generation.

Important existing facts:

- ABI: `AF`, `BC`, `DE`, `HL`, and `IY` are caller-saved. `IX` is
  callee-saved and used as the frame pointer.
- Return values come back in the main bank: `L`, `HL`, or `DE:HL`.
- Calls are therefore natural barriers for register allocation.
- `lib/runtime.s` already uses the alternate bank internally for
  `__div32` and `__mod32`, and documents that those helpers trash the
  alternate set.

So the backend is already conservative for good reasons, not because the
alternate set is useless.

## Hardware Reality

The Z80 alternate set is not one thing.
It is two very different mechanisms:

### `EX AF,AF'`

- swaps `A` with `A'`
- also swaps `F` with `F'`
- therefore moves data and flags together

This makes `A'` attractive for one-byte temps, but dangerous around
branches, compares, and any code that assumes flags survive.

### `EXX`

- swaps `BC`, `DE`, and `HL` with `BC'`, `DE'`, and `HL'`
- swaps all three pairs at once
- does not let us treat `BC'`, `DE'`, and `HL'` as independent ordinary
  registers

That last point matters most.
Trying to allocate "just `DE'`" or "just `HL'`" globally will make the
design brittle, because every `EXX` implicitly exchanges all three pairs.

## Main Constraints

Any better design has to respect these constraints.

### ABI and call constraints

- The current ABI does not promise preservation of alternate registers.
- External code cannot be assumed to preserve `AF'` or the `EXX` bank.
- Runtime helpers already use alternate registers as scratch.
- Therefore all alternate-register values should be considered dead at
  every call unless a future internal convention says otherwise.

### Liveness and control-flow constraints

- The current prepass uses simple instruction-index intervals.
- That is enough for one `BC` slot and one `A'` slot.
- It is not enough for safe `EXX` regions that cross labels, branches,
  or merge points.

### Flag constraints

- `ex af,af'` swaps `F` as well as `A`.
- Any branch or compare that relies on main flags must be emitted after
  flags are recomputed in the main bank.
- `A'` can be a data cache, but `F'` should be treated as garbage unless
  a very local sequence proves otherwise.

### Backend structure constraints

- Much of current codegen assumes `HL` is the primary register, `DE` is
  the secondary register, and `BC` is scratch.
- TLS access, shift loops, helper calls, and some address formation
  already consume `BC`.
- The peephole pass is textual and should not be asked to reason about
  alternate-bank liveness.

## Design Principles

The design should follow a few firm rules.

### 1. Keep the main bank canonical

The main bank should remain the default location for:

- call arguments
- return values
- condition production
- ordinary loads and stores
- function boundaries

Alternate registers should be an optimization layer, not a second
calling convention.

### 2. Treat `A'` and `EXX` separately

They should not share one vague "alternate register" abstraction.

Instead:

- `A'` is a single-byte cached temp with flag hazards
- `EXX` is a whole-bank region mechanism for up to three 16-bit values

### 3. Prefer region-based `EXX`, not independent `BC'/DE'/HL'` allocation

This is the core maintainability choice.

Do not extend the current string-based temp assignment model into
"`bc_alt`", "`de_alt`", and "`hl_alt`" as globally allocatable slots.
That model hides the fact that `EXX` swaps all three pairs together.

Instead, introduce explicit `EXX` regions with a clear entry and exit.

### 4. Calls, inline asm, and volatile operations stay as hard barriers

If a region touches:

- `CALL`
- indirect call trampoline usage
- `INLINE_ASM`
- volatile memory operations
- helper patterns known to use the alternate bank

then the region ends before that instruction.

### 5. Do not chase heroic global allocation

This backend does not need graph coloring for every Z80 register.
The best trade-off is a small number of safe, local, obvious wins.

## Proposed Model

### 1. Make clobbers explicit

Before expanding alternate-register use, introduce typed clobber masks.

Recommended categories:

- `clobber_main_af`
- `clobber_main_bc`
- `clobber_main_de`
- `clobber_main_hl`
- `clobber_alt_af`
- `clobber_alt_exx`
- `clobber_flags`
- `barrier_call`
- `barrier_cfg`

`clobber_alt_exx` means "this instruction or helper makes any current
`EXX`-bank contents invalid".

This is better than encoding safety rules indirectly through
`clobbers_bc()` and a few special cases.

### 2. Replace stringly temp locations with typed locations

The current `temp_regs_` string map is fine for two cases, but it will
not scale safely.

Use a small enum such as:

```cpp
enum class temp_home {
    stack,
    main_bc,
    alt_a,
    alt_bc,
    alt_de,
    alt_hl,
};
```

Even if `alt_bc`, `alt_de`, and `alt_hl` are only valid inside explicit
`EXX` regions, a typed representation makes the invariants visible.

### 3. Keep `A'` as a tiny, local optimization

`A'` is worth keeping and slightly improving, but it should remain
deliberately small in scope.

Recommended rules:

- still only one live temp in `A'`
- never live across `CALL`
- never live across `IFX`
- never assume `F'` has meaning
- never let main-bank flags stay live across `load_a()` or `store_a()`
  that use `ex af,af'`

This can be improved by making flag death explicit rather than relying on
comments alone.

### 4. Introduce `EXX` blocks as local scheduling regions

The most promising extension is not "allocate alternate pairs forever".
It is "create short basic-block-local `EXX` windows".

Inside such a window:

- the main bank continues to run the current code sequence
- one `EXX` moves us into the alternate triplet
- selected 16-bit temps live in `BC'`, `DE'`, and `HL'`
- a closing `EXX` returns to the main bank

The allocator should only build such a window when:

- it stays within one basic block
- it does not cross a call
- it does not cross inline asm
- it does not cross a control-flow split or merge
- it saves more reload/store traffic than the two `EXX` instructions cost

That makes the model understandable:

- `BC` allocation is global-ish and simple
- `A'` allocation is tiny and simple
- `EXX` is local and explicit

## Where `EXX` Blocks Can Help

The alternate triplet is most useful when the backend otherwise bounces
values through stack slots or push/pop sequences.

Good candidates:

- short arithmetic chains that reuse two or three 16-bit temps
- compare/lowering sequences that need one preserved value while `HL`
  and `DE` do work in the main bank
- local pointer arithmetic where a base pointer and an end pointer are
  both reused
- multiword operations lowered word-by-word, where one word pair can be
  parked off the main path

Less attractive candidates:

- code with many calls
- code with frequent flag-dependent branches
- large 32-bit/64-bit lowering paths that already rely on runtime
  helpers
- memory-heavy code where aliasing forces frequent reloads anyway

## Suggested `EXX` Region Formation

The region builder should stay simple.

### Phase-1 region rules

- only within a single basic block
- only for 16-bit temps
- no address-taken temps
- no values live across labels or branch targets
- no nested `EXX` regions
- no interaction with `A'` beyond normal clobber accounting

### Candidate heuristic

Build an `EXX` region only if all of the following hold:

- at least two 16-bit temps are live and reused in the block
- at least one of them would otherwise spill or reload more than once
- two `EXX` instructions plus setup are still cheaper than the avoided
  stack traffic

### Mapping policy

Inside a region:

- prefer `HL'` for the most frequently reused value
- prefer `DE'` for the next reused value
- use `BC'` last, because `BC`-shaped logic is often already special in
  current codegen and this keeps the mental model simpler

The exact order matters less than using one stable rule everywhere.

## Call Clobbers and Helper Policy

The safest near-term rule is:

### Every call clobbers all alternate state

That includes:

- direct calls
- indirect calls via `__call_hl`
- runtime arithmetic helpers
- TLS helper calls such as `__tls_base`

This matches the current ABI spirit and the runtime reality.
It also keeps external interoperability simple.

If the project later wants more, the next step should not be a silent
assumption that callees preserve alternates.
It should be an explicit internal-only convention for selected leaf
helpers, with documentation and tests.

## Liveness Requirements

The current interval prepass is enough for `BC` and mostly enough for
`A'`.
It is not the right tool for `EXX` blocks by itself.

Recommended minimum:

- split each function into basic blocks
- compute block-local backward liveness for temps
- identify call barriers and control-flow barriers
- only form `EXX` regions from that block-local data

This is still modest.
It is far smaller than a full global allocator, but strong enough to
avoid obvious cross-branch corruption.

## Risks

### Hidden flag bugs

`ex af,af'` is the easiest place to get a silent miscompile.
If alternate use expands, tests must cover:

- compare then branch
- byte temp in `A'` around compare production
- zero/sign extension paths
- nested boolean lowering

### False independence of `BC'/DE'/HL'`

The biggest design trap is pretending `EXX` gives three ordinary
registers.
It does not.
The implementation must preserve the notion that those registers are only
meaningful inside a bracketed bank-swapped region.

### Interaction with existing helpers

Current code relies on `BC` for:

- shift-count loops
- TLS offset addition
- helper-call cleanup patterns

An alternate-register design that ignores these patterns will either
lose its benefit or create ad hoc exceptions everywhere.

### Debuggability

If region formation becomes opaque, backend debugging gets painful fast.
For that reason, region creation should be deterministic and easy to log.

## What Should Stay Simple

These are good places to stop.

- Do not preserve alternate state across calls.
- Do not allocate alternate registers across basic-block boundaries in
  the first implementation.
- Do not teach the peephole pass about alternate-register liveness.
- Do not build a global graph-coloring allocator for the Z80 backend.
- Do not try to make `F'` a meaningful long-lived value.
- Do not use alternate registers for 32-bit or 64-bit values at first.
- Do not let inline asm participate unless it declares full clobbers.

Those limits are not a weakness.
They are what keeps the feature maintainable.

## Phased Implementation Plan

### Phase 0: Document and harden the current rules

- Add explicit documentation that alternate registers are caller-saved
  scratch from the compiler's perspective.
- Record that `__div32` and `__mod32` clobber the alternate set.
- Replace string temp-location tags with a small enum.
- Introduce explicit clobber/barrier classification helpers.

Expected result:

- no new optimization yet
- cleaner foundation
- fewer accidental assumptions

### Phase 1: Strengthen current `BC` and `A'` allocation

- Keep current one-slot `BC` allocation.
- Keep current one-slot `A'` allocation.
- Make flag invalidation explicit when `A'` is touched.
- Add tests for `A'` around `IFX`, compare lowering, casts, and calls.

Expected result:

- safer version of what already exists
- minimal code churn

### Phase 2: Add basic-block-local `EXX` regions

- Build basic blocks and block-local liveness.
- Detect profitable short regions with 2-3 reusable 16-bit temps.
- Enter region with `EXX`, use `BC'/DE'/HL'`, exit with `EXX`.
- Spill or abandon the region at any barrier.

Expected result:

- real improvement beyond current `BC`/`A'`
- still understandable in dumps and code review

### Phase 3: Add a few targeted patterns, not a general revolution

- Improve common arithmetic or pointer-heavy templates that benefit from
  parking values in the alternate triplet.
- Optionally allow selected internal leaf helpers to promise alternate
  clobber behavior explicitly.

Expected result:

- focused wins where profiling or assembly inspection shows value
- no pressure to redesign the whole backend

## Recommended Non-Goals

The project should explicitly avoid these unless there is later strong
evidence they are worth the complexity:

- preserving alternate state across arbitrary calls
- cross-block or loop-global `EXX` allocation
- alternate-register-aware peephole rewriting
- treating `F'` as an allocatable condition-code resource
- full global register allocation for all Z80 registers

## Summary

The best path forward is conservative but useful:

- keep the main bank as the canonical ABI-visible state
- keep `A'` as a tiny byte-temp cache with strict flag rules
- treat `EXX` as a region mechanism, not as three independent registers
- make alternate clobbers explicit
- limit the first real expansion to basic-block-local `EXX` regions

That approach matches the current backend architecture.
It gives room for meaningful optimization beyond today's limited
`BC`/`A'` use, while keeping the compiler readable and maintainable.
