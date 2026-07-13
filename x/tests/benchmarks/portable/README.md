# Portable Cross-Compiler Benchmarks

This suite is a calibrated cross-compiler benchmark matrix meant to answer a
portable question first:

- which self-checking C kernels execute correctly across the whole compiler set
- how `xcc -Os` compares on code size and `xcc -Of`/`-O3` compare on emulator cycles once correctness is fixed
- which portable cases are still speed violators after removing miscompile bait

The current suite is generated from conservative, libc-free C programs so the
same benchmark can be compiled and executed by:

- `xcc -Of`, `xcc -O3`, and `xcc -Os`
- `sdcc --opt-code-size`
- `sdcc --opt-code-speed`
- `z88dk` `80cc`
- `z88dk` `sccz80`

Layout:

- `include/portable_bench.h` — shared helpers used by every generated source
- `generate_portable_benchmarks.py` — regenerates the calibrated portable suite
- `extract_common_subset.py` — still useful for experimental corpora and ad hoc result reduction
- `render_portable_summary.py` — turns CSV results into the markdown report
- `generated/` — the generated benchmark sources
- `expected.csv` — return-code oracle for the runner

The active generator emits 4 families with 10 variants each, for 40 total
benchmarks. All four families share the same run-scanning control-flow shape,
because broader kernels still exposed real compiler/runtime bugs during
admission:

- `rle_short`
- `rle_mixed`
- `rle_dense`
- `rle_wide`

Regenerate the suite with:

```bash
python3 x/tests/benchmarks/portable/generate_portable_benchmarks.py
```

Run it with:

```bash
bash x/tests/run_benchmarks.sh --suite portable
```

The most recent validated matrix was written to:

```bash
build/x/benchmarks/portable_final_v1/portable/summary.md
```

If you experiment with a broader corpus, reduce a finished matrix to only the common-pass cases with:

```bash
python3 x/tests/benchmarks/portable/extract_common_subset.py \
  --results build/x/benchmarks/portable_full/results.csv \
  --outdir build/x/benchmarks/portable_full
```
