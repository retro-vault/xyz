# Optimization Research Sources

This note keeps the external optimization references we are mining for the
research queue. `-O3` is now an exact alias of `-Of`, so unfinished ideas
remain manual or out of tree until stabilized; this list is not a promise
that every idea is safe or directly portable to `xcc`.

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

- [Austin Henley, "Superoptimizer"](https://austinhenley.com/blog/superoptimizer.html)
  A theoretical/practical introduction to superoptimization.  Useful as
  inspiration for generating candidate Z80 rewrites, then validating them
  with exact flag/register semantics and the executable benchmark oracle.

## Local Rules For Using These Sources

- Imported ideas remain manual experiments until they are proven safe enough
  to enter `-Of`; `-O3` does not provide a separate staging lane.
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
