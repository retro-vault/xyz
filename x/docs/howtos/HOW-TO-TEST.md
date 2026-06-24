# How to Test

This guide reflects the current repository test entry points.

## Main End-to-End Wrapper

Run the full regression wrapper from the repository root:

```sh
./tests/e2e_test.sh
```

This builds the toolchain first, then runs the currently wired end-to-end
phases. Exit code `0` means every selected phase passed.

If the binaries are already current:

```sh
./tests/e2e_test.sh --no-build
```

To run one phase only:

```sh
./tests/e2e_test.sh --no-build --phase xz80
./tests/e2e_test.sh --no-build --phase xcc
./tests/e2e_test.sh --no-build --phase xcc-exec
./tests/e2e_test.sh --no-build --phase xld
./tests/e2e_test.sh --no-build --phase xas
./tests/e2e_test.sh --no-build --phase xar
./tests/e2e_test.sh --no-build --phase xgdb
./tests/e2e_test.sh --no-build --phase xemu
./tests/e2e_test.sh --no-build --phase mdr
./tests/e2e_test.sh --no-build --phase chain
```

Current phase names are:

- `build`
- `xz80`
- `xcc`
- `xcc-exec`
- `xld`
- `xas`
- `xar`
- `xgdb`
- `xemu`
- `mdr`
- `chain`

## What Each Phase Covers

| Phase | What it covers | Notes |
|-------|----------------|-------|
| `build` | Host-tool rebuild for the core toolchain pieces used by the suite | Only runs unless `--no-build` is passed |
| `xz80` | `lib/xz80` unit tests | CPU/disassembler library validation |
| `xcc` | `x/tests/run_tests.sh --filter xcc` | Unified manifest-driven compiler compile/run regressions |
| `xcc-exec` | `x/tests/run_tests.sh --filter xcc_exec_` | Execution-oriented compiler regressions with xemu and optional host gcc goldens |
| `xld` | `src/xc/xld` tests | Linker parsing, placement, relocation, and emission |
| `xas` | `x/tests/tests/xas/asm_compare_test.sh` plus `make -C x/src/xas test` | Parity against `sdasz80` plus format-conversion coverage |
| `xar` | Archive smoke tests inside the wrapper | Create, list, and link-against archive flow |
| `xgdb` | `lib/xgdb` tests | Debugger library-level coverage |
| `xemu` | `lib/xemu` plus `src/xemu` smoke tests | Emulator library coverage plus direct stdio execution |
| `mdr` | `tests/mdr-emu` | Microdrive end-to-end validation |
| `chain` | Full `xcc -> xas -> xld` integration | Verifies valid XL binary output on representative programs |

## Useful Focused Commands

You do not need to use the wrapper for every iteration.

Common direct commands:

```sh
make -C lib/xz80 test
make -C src/xc/xas test
make -C src/xc/xas test-libs
make -C src/xc/xld test
make -C lib/xgdb test
make -C lib/xemu test
make -C src/xemu test
make -C y/tests/mdr-emu test
bash x/tests/run_tests.sh ./bin/x/bin/xcc --filter xcc
bash x/tests/run_tests.sh ./bin/x/bin/xcc --filter xcc_exec_
make -C src/xc/xcc bench
```

## Codegen Benchmarks

For a repeatable compiler-size benchmark against SDCC that measures only
generated translation-unit code, use:

```sh
bash archive/x/tests/tests/xcc-legacy/run_codegen_bench.sh ./bin/x/bin/xcc
```

By default this benchmarks the `exec/int` suite with `sdcccall(1)` and
records `xcc -O0/-O1/-O2/-Of/-O3/-Os` against SDCC `--opt-code-size`
and `--opt-code-speed`. If SDCC cannot compile an xcc-specific probe,
that row is recorded as `n/a` for SDCC and kept in the xcc totals.

Outputs land under:

```sh
build/xc/xcc/bench/codegen/int/
```

The benchmark writes:

- `results.csv` with per-test byte counts
- `summary.md` with totals, deltas, `-Os` versus `-O2`, and `-O3` versus
  `-Os` comparisons
- `versions.txt` with the exact tool versions used

To cover all exec suites instead of only integer tests:

```sh
bash archive/x/tests/tests/xcc-legacy/run_codegen_bench.sh ./bin/x/bin/xcc --suite all
```

## Practical Refactoring Checklist

1. Make the code change.
2. Run the smallest direct test that covers the touched area.
3. Run `./tests/e2e_test.sh --no-build` before calling the change done.
4. If you changed build wiring or staged outputs, rerun the wrapper without `--no-build`.
5. Commit only when the relevant direct tests and the wrapper are green.

## Prerequisites and Skip Behavior

- `sdasz80` must be on `PATH` for the `xas` parity phase.
  If it is missing, that phase is skipped.

- `make -C src/xc/xas test-libs` is a heavier library round-trip parity sweep.
  It also needs the GNU Z80 assembler (`${Z80_GNU_PREFIX}as` by default).

- The `xcc-exec` phase needs:
  `sdasz80`, `sdldz80`, `g++`, and the configured Z80 GNU binutils
  (`z80-unknown-elf-as`, `z80-unknown-elf-ld`, `z80-unknown-elf-objcopy`
  by default).

- `python3` is needed for the `chain` phase XL-header checks.

- The wrapper disables LeakSanitizer because the current execution harness is
  ptrace-based. AddressSanitizer and UBSan remain active.
