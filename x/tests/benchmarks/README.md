# X Benchmarks

All X-side benchmarks live under this single root.

Layout:

- `bare/` — standalone bare-metal kernels compared across `xcc` and `sdcc`
- `numeric/` — numeric-format matrix benchmarks (`fixed8_8`, `fixed16_16`, `fixed24_8`, `float`, `double`)
- `portable/` — calibrated 40-case portable matrix for xcc size/speed profiles vs SDCC and z88dk
- `z88dk/` — libc-free adaptations of the z88dk compiler-comparison kernels
- `grouped/` — grouped or future multi-file benchmark material

The active benchmark entrypoint is:

- `x/tests/run_benchmarks.sh` — runs the active suites under this root

Useful options:

- `--suite bare`
- `--suite numeric`
- `--suite portable`
- `--suite z88dk`
- `--filter <regex>`
- `--bare-filter <regex>`
- `--numeric-filter <regex>`
- `--portable-filter <regex>`

The runner prints markdown tables directly in the terminal and writes its
detailed reports under `build/x/benchmarks/` by default.
