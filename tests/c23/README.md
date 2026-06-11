# C23 Compatibility Suite

This repository builds a practical, conformance-oriented C23 feature test
pack. It is not an ISO official test suite, but it is designed to map
directly onto the major C23 language, preprocessor, library, and IEC 60559
feature families documented in the public WG14 draft.

## Layout

- `docs/dist/` holds published matrix documents.
- `docs/research/` holds the generated feature catalog.
- `docs/sources/` archives the standards and support references used to build
  the suite.
- `tests/spec/` holds the suite source-of-truth catalog.
- `tests/scripts/` holds the generator and matrix runner.
- `tests/cases/` holds generated feature folders.
- `tests/data/` holds shared test data.
- `build/results/` holds raw compiler runs and JSON reports.

## Build And Run

Generate the suite:

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
