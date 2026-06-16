# Benchmarks

This project now has two different benchmark flows:

- a **codegen-only** benchmark that measures translation-unit `_CODE` bytes
  before linking
- a **bare-metal executable** benchmark that links a tiny `crt0`, runs under
  the Z80 emulator, and records both payload size and cycle count

They answer different questions, so both are worth keeping.

## Codegen-Only Benchmark

Use this when you want to compare compiler output before runtime helpers,
linker layout, or startup code are mixed in.

Run it with:

```sh
bash src/xc/xcc/tests/run_codegen_bench.sh ./bin/x/bin/xcc
```

Important properties:

- input suite: `src/xc/xcc/tests/data/exec/<suite>`
- metrics: object `_CODE` bytes only
- no CRT
- no runtime helper objects
- no emulator execution

Default output:

```sh
build/xc/xcc/bench/codegen/<suite>/
```

## Bare-Metal Executable Benchmark

Use this when you want something closer to a real tiny-Z80 program:

- standalone executable image
- no libc
- no `printf`
- shared minimal `crt0`
- self-checking return code
- emulator cycle count

Run it with:

```sh
bash tests/run_benchmarks.sh ./bin/x/bin/xcc
```

Useful options:

```sh
bash tests/run_benchmarks.sh ./bin/x/bin/xcc --filter 'crc16|vm_dispatch'
bash tests/run_benchmarks.sh ./bin/x/bin/xcc --cycles 10000000
bash tests/run_benchmarks.sh ./bin/x/bin/xcc --outdir /tmp/xyz-bench
```

Default output:

```sh
build/benchmarks/
```

Important files:

- `results.csv`
  Per-benchmark status, expected checksum, measured checksum, payload bytes,
  and cycles for every mode.
- `summary.md`
  Human-readable totals and comparisons.
- `versions.txt`
  Exact tool versions plus the active cycle budget.
- `tests/benchmarks/expected.csv`
  Source-semantics oracle values for every benchmark.
- `work/`
  Intermediate `.s`, `.rel`, `.ihx`, and `.bin` files for inspection.

### What The Runner Measures

Each benchmark is built six ways:

- `xcc -O2`
- `xcc -Of`
- `xcc -O3`
- `xcc -Os`
- `sdcc --opt-code-size --fomit-frame-pointer --sdcccall 1`
- `sdcc --opt-code-speed --fomit-frame-pointer --sdcccall 1`

The runner records:

- `status`
  `ok`, `timeout`, `build_error`, or `link_error`
- `return`
  the 16-bit checksum returned by `main()`
- `match`
  whether the returned checksum matches the oracle in
  `tests/benchmarks/expected.csv`
- `payload bytes`
  flat linked binary size minus the shared `crt0` bytes
- `cycles`
  cycle count reported by `build/bin/z80_exec`

This is still a deliberately small-system benchmark:

- no libc
- no formatted output
- no OS
- a tiny mailbox-based `crt0`
- a fixed cycle budget per image

### Benchmark Set

Each benchmark lives in its own subdirectory under `tests/benchmarks/`.

Current workload set:

- `binary_search`
- `counting_sort`
- `crc16`
- `fir_shiftadd`
- `flood_fill`
- `gray_decode`
- `histogram`
- `insertion_sort`
- `life_step`
- `list_sort`
- `matrix_mix`
- `nibble_lut`
- `pointer_chase`
- `ring_buffer`
- `rle_encode`
- `sieve_bits`
- `state_machine`
- `token_scan`
- `vm_dispatch`
- `window_minmax`

These are intentionally original, stripped-down kernels, not direct ports of
CoreMark, Embench, BEEBS, or MiBench. The goal is to keep them small,
library-free, and easy to link and execute in the current test harness while
still covering similar workload families:

- CRC and bit manipulation
- list, search, and sort kernels
- state-machine and parser-style branchy code
- table lookups and pointer chasing
- matrix-like and cellular update kernels
- tiny interpreter / dispatch loops

### Current State

The executable suite is intentionally the reality check on flattering
translation-unit-only size numbers.

At the time this guide was last updated, the default full run showed:

- `xcc -O2`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -Of`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -O3`: `20 / 20` successful runs, `20 / 20` correct checksums
- `xcc -Os`: `20 / 20` successful runs, `20 / 20` correct checksums
- `sdcc --opt-code-size`: `19 / 20` successful runs, `19 / 20` correct checksums
- `sdcc --opt-code-speed`: `20 / 20` successful runs, `20 / 20` correct checksums

So the benchmark story has changed a lot:

- `xcc` is now functionally correct on the full benchmark oracle set
- `-Of`, `-O3`, and `-Os` are all benchmark-clean on this suite
- the proven aggressive baseline has now been promoted, so `-Of`, `-O3`,
  and `-Os` currently measure the same on the executable benchmark set
- `-O2` is still the more conservative general preset
- the remaining benchmark problem is no longer correctness
- the next benchmark task is to keep `-O3` available for new
  experiments without regressing the shared promoted baseline
Treat the executable suite as the better “trust but verify” tool when
codegen-only size numbers look too good to be true, but now interpret it
primarily as a **performance, size, and stabilization** benchmark rather
than a correctness triage harness.

### Interpreting Status

- `ok`
  The image linked, returned through the mailbox, and stayed within the cycle
  budget. Check the `match` column to see whether the checksum is also correct.
- `timeout`
  The image linked and ran, but did not finish before the cycle budget.
- `build_error`
  The compiler or assembler failed.
- `link_error`
  The object linked poorly or referenced unsupported helpers.

The summary compares sizes and cycles only on benchmark rows where both sides
completed successfully **and** matched the oracle checksum. This avoids fake
“wins” caused by timeouts or wrong answers.

## Design Notes

The executable suite is inspired by the same broad ideas used in modern
embedded benchmark suites:

- Embench emphasizes no OS, minimal C library assumptions, and no output
  stream for deeply embedded systems:
  https://www.embench.org/
- The Embench repository describes the suite as targeting deeply embedded
  systems with no OS and no output stream:
  https://github.com/embench/embench-iot
- CoreMark’s published workload families are list processing, matrix
  manipulation, state machines, and CRC:
  https://www.eembc.org/coremark/
- BEEBS explicitly targets deeply embedded systems and likewise assumes no OS
  and no output stream:
  https://beebs.mageec.org/
- MiBench is a broader embedded benchmark suite and remains a useful source of
  workload categories:
  https://vhosts.eecs.umich.edu/mibench/
- Dhrystone is historically important, but it is too narrow by itself for the
  sort of cross-checking we want now:
  https://www.keil.com/benchmarks/dhrystone.asp

We are not shipping or claiming official CoreMark / Embench / BEEBS / MiBench
results. We are using their workload categories and bare-metal assumptions as
guidance for a small, repo-local regression and comparison suite.

## Caveats

- The codegen-only benchmark and the executable benchmark are not interchangeable.
- The executable benchmark is currently better at finding real optimizer bugs.
- The cycle totals depend on the emulator and the current `crt0`.
- Some SDCC images are slightly larger or smaller purely because of different
  prologue choices, not because of the kernel body alone.
- If any mode returns the wrong checksum, treat that as a correctness bug
  first and a benchmarking result second.
- If `xcc` times out on a benchmark, that is a meaningful result and should
  not be silently normalized away.
