# X Benchmarks

All X-side benchmarks live under this single root.

Layout:

- `bare/` — standalone bare-metal kernels compared across `xcc` and `sdcc`
- `numeric/` — numeric-format matrix benchmarks (`fixed8_8`, `fixed16_16`, `fixed24_8`, `float`, `double`)
- `portable/` — calibrated 40-case portable matrix for xcc size/speed profiles vs SDCC and z88dk
- `z88dk24/` — 23 captured upstream full-program integer benchmarks plus the
  host-verified `bitfieldbench`, using one pinned z88dk `+test`
  CRT/classic-library baseline and XCC's M distribution
- `grouped/` — grouped or future multi-file benchmark material

The active benchmark entrypoint is:

- `x/tests/run_benchmarks.sh` — runs the active suites under this root

Useful options:

- `--suite bare`
- `--suite numeric`
- `--suite portable`
- `--suite z88dk` (canonical full-program corpus)
- `--suite z88dk24` (backward-compatible alias)
- `--filter <regex>`
- `--bare-filter <regex>`
- `--numeric-filter <regex>`
- `--portable-filter <regex>`

The runner prints markdown tables directly in the terminal and writes its
detailed reports under `build/x/benchmarks/` by default.

The full upstream comparison builds and runs seven compiler/profile
combinations per program. Prepare its three locked z88dk checkouts once, then
run it directly:

```sh
x/tests/benchmarks/z88dk24/prepare.sh
x/tests/benchmarks/z88dk24/run.sh
```

The seven lanes are sccz80, XCC M `-Os`, XCC M `-Of`, current zsdcc,
the limited `sdcc-max` probe, and current 80cc with and without its frame
pointer. See `z88dk24/README.md` for exact commits, flags, compatibility
patches, the lazy 23-byte XCC register-ABI multiply adapter, and the final
24/24 XCC correctness results. Current zsdcc's `bitfieldbench` failure remains
visible rather than being excluded from the report.
