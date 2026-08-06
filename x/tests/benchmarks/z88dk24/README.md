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
`spectralnorm` program is intentionally excluded because it requires the
double support that the M-model comparison is designed to omit.

XCC is benchmarked with the M distribution (`bin/x-m`): `long` remains
available, while `double`, `long long`, and stdio floating conversions are
excluded. This matches the intended integer-only comparison.

The harness records six XCC lanes: `-Os`, `-Of`, and `-O3` with the default
`sdcccall(1)` ABI, plus matching `--sdcccall 0` runs. Keeping `-O3` explicit
ensures that new profile-specific optimization work is measured directly,
even when a particular release makes it equivalent to `-Of`.
All XCC and z88dk-family images now execute in the same `z80_exec` Z80 model.
The runner implements z88dk's test-CRT trap protocol for console and file I/O,
so cycle comparisons no longer mix emulator models.

`compat/` contains only compiler/platform adapters needed to build an
otherwise unchanged upstream source. In particular, the historical MD5
source omits POSIX headers and calls the three-argument form of `open`.

Run the complete comparison with:

```sh
bash x/tests/benchmarks/z88dk24/run.sh
```

The default output is `build/x/benchmarks/z88dk24/`.  Every image actually
uploaded to an emulator is copied to
`artifacts/<benchmark>/<lane>/program.bin`; the byte value in `results.csv`
and `summary.md` is exactly `wc -c` for that file.  XCC map files are kept
beside its binaries, and `work/<benchmark>/` retains build and run logs.

See [RESULTS.md](RESULTS.md) for the audited baseline and the graduated
2026-08-06 profiles, including exact pins, the overfitting ablation,
measurement caveats, and validation totals.
