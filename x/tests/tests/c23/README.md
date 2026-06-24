# C23 Test Root

This directory is the single home for the `x` C23/compiler-facing tests.

It contains both:

- the active unified `xemutest` suite that `x/tests/run_tests.sh` executes
- the imported C23 compatibility matrix used for broader survey and research

The imported compatibility matrix is not an ISO official test suite, but it is
designed to map directly onto the major C23 language, preprocessor, library,
and IEC 60559 feature families documented in the public WG14 draft.

## Layout

- `cases/` holds the active manifest-driven `xemutest` cases
- `cases/probes/` holds hand-authored smoke and debugger probes
- `cases/xcc/` holds generated `xcc` cases grouped by area
- `xcc/data/` holds the shared source payloads reused by those cases
- `corpus/` holds imported corpus projects and upstream snapshots
- `tests/spec/` holds the imported compatibility suite source-of-truth catalog
- `tests/scripts/` holds the imported suite generator and matrix runner
- `tests/cases/` holds imported compatibility feature folders
- `tests/data/` holds imported compatibility shared test data
- `docs/dist/` holds published matrix documents
- `docs/research/` holds the generated feature catalog
- `docs/sources/` archives standards and support references
- `build/results/` holds raw compiler runs and JSON reports

## Build And Run

Run the active unified suite:

```sh
XCC=bin/x/bin/xcc bash x/tests/run_tests.sh
```

Generate the imported compatibility suite:

```sh
make generate
```

Run the matrix with the default compiler:

```sh
make matrix
```

Run the matrix with a different compiler:

```sh
make matrix CC=clang
```

Run the matrix through a compiler profile:

```sh
make matrix PROFILE=tests/config/profiles/gcc.json
```

Run the SDCC Z80 setup:

```sh
make matrix-sdcc-z80
```

Or explicitly:

```sh
make matrix PROFILE=setups/sdcc-z80/profile-ucsim.json
```

Compile only, without executing runnable tests:

```sh
make matrix PROFILE=tests/config/profiles/gcc.json RUN_MODE=never
```

## Output

- The feature catalog is written to `docs/research/c23-feature-catalog.md`.
- The published matrix is written to `docs/dist/c23-compatibility-matrix.md`.
- The raw machine-readable results are written to `build/results/`.

Detailed compiler onboarding notes live in `docs/howtos/TEST.md`.
The SDCC Z80 setup bundle lives in `setups/sdcc-z80/`.
