# z88dk Full-Program Integer Benchmark Corpus

This directory vendors the complete compiler-comparison corpus from z88dk's
`80cc-codegen` branch at commit
`460ec34769f01324ca49b66145f09babdbe507fc` (2026-07-13).

Unlike the older `../z88dk/` directory, these are the original full-program
sources. They run with z88dk's `test.h` framework and link each compiler's
CRT and integer-formatting C library. Binary size is the complete linked
image; no empty-program baseline is subtracted.

The upstream comparison is commonly referred to as the 24-test corpus, but
the referenced branch and published table contain 23 integer programs. This
directory includes every one of those rows. The separate floating-point
`spectralnorm` program is intentionally excluded because it requires the
double support that the M-model comparison is designed to omit.

XCC is benchmarked with the M distribution (`bin/x-m`): `long` remains
available, while `double`, `long long`, and stdio floating conversions are
excluded. This matches the intended integer-only comparison.

The harness now records four XCC lanes: `-Os` and `-Of` with the default
`sdcccall(1)` ABI, plus matching `-Os --sdcccall 0` and `-Of --sdcccall 0`
runs so stack-ABI results can be compared directly against the same corpus.

`compat/` contains only compiler/platform adapters needed to build an
otherwise unchanged upstream source. In particular, the historical MD5
source omits POSIX headers and calls the three-argument form of `open`.

Run the complete comparison with:

```sh
bash x/tests/benchmarks/z88dk24/run.sh
```
