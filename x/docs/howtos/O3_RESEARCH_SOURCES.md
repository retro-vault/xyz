# Optimization Research Sources

This note keeps the external optimization references we are mining for the
research queue. `-O3` is currently an empty experimental alias of `-Of`, ready
for the next speed investigation; ideas still require legality proofs, general
tests, and benchmark validation.
This list is not a promise that every idea is safe or directly portable to
`xcc`.

## Z80 Optimizer Projects

- [`santiontanon/mdlz80optimizer`](https://github.com/santiontanon/mdlz80optimizer)
  A Z80 assembly optimizer to study for peephole databases, register-flow
  reasoning, and repeated instruction-sequence cleanup ideas.

- [`avwohl/uc80`](https://github.com/avwohl/uc80)
  A Z80-oriented compiler/tool project to inspect for code-generation
  patterns, small-machine IR choices, and backend simplification tricks.

- [`oisee/z80-optimizer`](https://github.com/oisee/z80-optimizer)
  A Z80 optimizer project to mine for local assembly rewrites and rule
  organization ideas that could become `-O3` peepholes after testing.

- [`omarandlorraine/strop`](https://github.com/omarandlorraine/strop)
  A superoptimizer-style project to inspect for search strategies and ways
  to discover tiny exact rewrites beyond hand-written peepholes.

## Superoptimizer Background

- [Souper: A Synthesizing Superoptimizer](https://research.google/pubs/souper-a-synthesizing-superoptimizer/)
  demonstrates synthesizing small IR improvements and then generalizing the
  discoveries into ordinary compiler transformations.  For XCC that means a
  discovered Z80 sequence becomes a typed, source-independent IR or machine
  rule; the benchmark itself never becomes a selector.

- [STOKE: A Stochastic Superoptimizer and Program Synthesizer](https://arxiv.org/abs/1211.0557)
  treats correctness and a target cost function as separate requirements for
  loop-free instruction sequences.  XCC uses the same separation when a local
  rewrite must preserve Z80 registers, flags, memory, and stack effects before
  its byte/cycle cost is considered.

- [Hydra: Generalizing Peephole Optimizations with Program Synthesis](https://2024.splashcon.org/details/splash-2024-oopsla/27/Hydra-Generalizing-Peephole-Optimizations-with-Program-Synthesis)
  is a useful model for turning one observed missed peephole into a broader
  rewrite guarded by dataflow facts rather than copying the observed program.

- [Austin Henley, "Superoptimizer"](https://austinhenley.com/blog/superoptimizer.html)
  A theoretical/practical introduction to superoptimization.  Useful as
  inspiration for generating candidate Z80 rewrites, then validating them
  with exact flag/register semantics and the executable benchmark oracle.

## Production Compiler References

- [LLVM Code Generator](https://llvm.org/docs/CodeGenerator.html) separates
  instruction selection, machine-level optimization, register allocation, and
  late peepholes.  XCC follows the same useful boundary: typed algebra belongs
  in IR, physical-home decisions belong in the Z80 backend, and final local
  instruction cleanup belongs in xopt.

- [LLVM InstCombine Contributor Guide](https://llvm.org/docs/InstCombineContributorGuide.html)
  emphasizes canonical, target-independent combines and demanded/known-bit
  reasoning. Target-specific reversals belong later, where XCC can attach Z80
  byte and cycle costs.

- [GCC peephole definitions](https://gcc.gnu.org/onlinedocs/gccint/Peephole-Definitions.html)
  document why post-allocation peepholes can exploit concrete registers but
  must respect incomplete local dataflow. This supports keeping XCC's
  cross-block recurrence optimization in structured code generation rather
  than an unproved textual assembly rule.

- [SDCC Compiler User Guide](https://sdcc.sourceforge.io/doc/sdccman.pdf)
  documents its target peephole machinery and speed/size policy. Its emitted
  Z80 remains a useful independent comparison, but an XCC change is admitted
  only from a general C/IR property and independent correctness cases.

## Manual Z80 Kernel Audit (2026-08-06)

The audit translated several ordinary C kernels by hand before changing the
compiler:

- A four-byte conditional shift recurrence naturally loads the state once,
  runs `srl d / rr e / rr h / rr l`, conditionally XORs the polynomial bytes,
  and repeats in `DEHL`. XCC previously committed all four bytes to the IX
  frame after every step even though the next proven diamond immediately
  consumed the same state. The experiment commits only after the final
  adjacent step. On the full CRC program this changed 124.3M measured cycles
  to 86.1M; after validation the rewrite graduated to `-Of` and, because it is
  strictly smaller as well as faster, to `-Os`.
- An RLE scanner wants an input cursor, output cursor/index, current byte, and
  run count resident across its two loops. Physical homes plus scalar
  promotion produce that general layout without recognizing RLE; the measured
  full program changed from 21.5M to 9.4M cycles during the O3 experiment. The
  guarded allocator and promotion have now graduated to `-Of`.
- A five-point matrix stencil wants affine row pointers and induction state in
  pairs rather than repeated IX reloads. The same allocation policy changes
  36.3M cycles to 32.7M.
- Dense bytecode dispatch is a counterexample: scalar promotion lengthened a
  selector live range and regressed a representative interpreter from 38.4M
  to 45.9M. A generic four-or-more-case `EQ`/`IFX` chain guard restores 38.4M;
  it does not inspect function names, source paths, opcodes, or outputs.
- The first broad holdout run rejected two apparently fast results rather than
  treating wrong output as a win. A copied outer induction variable was being
  followed textually even though an inner-loop decrement could reach the use
  through a backedge; the lockstep pointer pass now requires the followed
  definition to have no competing definition in the natural loop. Separately,
  xopt treated `ld a,N(ix); ld N(ix),a` as having no observable effect, but
  the load still defines `A`; cleanup is now limited to compiler-described
  temporary-frame slots and removes only the store unless all paths overwrite
  `A` before a read. Source-local slots remain intact without volatility
  metadata. These are source-independent dataflow fixes, with focused
  regressions, not exclusions for the programs that exposed them.

The rejected experiments are part of the result too. Broad promotion of
multi-definition signed locals did not create safe physical homes for binary
search and retained the known CFG risk, so it was removed instead of adding a
search-shaped exception.

## Local Rules For Using These Sources

- Imported ideas remain manual experiments until their legality and benefit
  are independently established. The now-empty `-O3` profile is the staging
  lane; only broadly validated speed work can later move to `-Of`.
- `-Os` remains the protected size-record lane.
- Every rewrite needs a focused compiler fixture and the executable
  correctness suite before it is considered trustworthy.
- Prefer tiny exact rules first: register value, flags, memory, stack depth,
  and control-flow behavior must all be accounted for.
- Use an explicitly designated development corpus for cost tuning. A frozen
  holdout may validate generalization once; do not derive a new selector from
  its functions or repeatedly tune against its score.
- Compare both `xcc` and competitors in one runner and record correctness,
  size, and cycles. A speed change is not releasable merely because it wins
  the development benchmark.
