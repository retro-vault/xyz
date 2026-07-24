# X Benchmarks

All X-side benchmarks live under this single root.

Layout:

- `bare/` — standalone bare-metal kernels compared across `xcc` and `sdcc`
- `numeric/` — numeric-format matrix benchmarks (`fixed8_8`, `fixed16_16`, `fixed24_8`, `float`, `double`)
- `portable/` — calibrated 40-case portable matrix for xcc size/speed profiles vs SDCC and z88dk
- `z88dk/` — libc-free adaptations of the z88dk compiler-comparison kernels
- `z88dk24/` — the complete 23-row upstream full-program integer corpus,
  using each compiler's CRT/libc and XCC's M distribution
- `grouped/` — grouped or future multi-file benchmark material

The active benchmark entrypoint is:

- `x/tests/run_benchmarks.sh` — runs the active suites under this root

Useful options:

- `--suite bare`
- `--suite numeric`
- `--suite portable`
- `--suite z88dk`
- `--suite z88dk24`
- `--filter <regex>`
- `--bare-filter <regex>`
- `--numeric-filter <regex>`
- `--portable-filter <regex>`

The runner prints markdown tables directly in the terminal and writes its
detailed reports under `build/x/benchmarks/` by default.

The full upstream comparison builds and runs eight compiler/profile
combinations per program. It is available through the unified runner above
or directly through:

- `x/tests/benchmarks/z88dk24/run.sh`
