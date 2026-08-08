# z88dk Full-Program Integer Benchmark Corpus

This directory vendors the complete compiler-comparison corpus from z88dk's
`80cc-codegen` branch at commit
`460ec34769f01324ca49b66145f09babdbe507fc` (2026-07-13).

These are the original full-program sources. They run with z88dk's `test.h`
framework and link each compiler's CRT and integer-formatting C library.
Binary size is the complete linked image; no empty-program baseline is
subtracted.

The upstream comparison is commonly referred to as the 24-test corpus, but
the referenced branch and published table contain 23 integer programs. This
directory includes every one of those rows. The separate floating-point
`spectralnorm` program is intentionally excluded because it is not one of the
23 rows in the frozen integer comparison.

The harness records XCC `-Os` and `-Of`, SDCC, and 80CC's frame-pointer and
stack-pointer lanes. Every compiler links the same z88dk `+test` CRT and
classic library, making both size and cycle results directly comparable.
All images execute in the same `z80_exec` Z80 model.
The runner implements z88dk's test-CRT trap protocol for console and file I/O,
so cycle comparisons no longer mix emulator models.

`compat/` contains only compiler/platform adapters needed to build an
otherwise unchanged upstream source. In particular, the historical MD5
source omits POSIX headers and calls the three-argument form of `open`.

Run the complete comparison with:

```sh
Z88DK=/path/to/patched/z88dk bash x/tests/benchmarks/z88dk24/run.sh
```

The default output is `build/x/benchmarks/z88dk24/`.  Every image actually
uploaded to the emulator is copied to
`artifacts/<benchmark>/<lane>/program.bin`; the byte value in `results.csv`
and `summary.md` is exactly `wc -c` for that file.  XCC map files are kept
beside its binaries, and `work/<benchmark>/` retains build and run logs.

`reference.csv` is the frozen 23-row shared-z88dk comparison supplied for
optimization work.  Treat it as the canonical before table; do not compare
new XCC results against the older private-runtime or SCCZ80 matrices.

See [RESULTS.md](RESULTS.md) for the patched-z88dk setup, exact pins,
measurement rules, and the current optimization result.
