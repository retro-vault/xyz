# xcc Testing Guide

## Current Test Infrastructure

### End-to-end assembly snapshot tests

Location: `tests/data/core/`

Each test is a pair of files:

- `tNNN_name.c` — C source input
- `tNNN_name.expected` — expected sdasz80 assembly output

The test runner (`tests/run_tests.sh`) compiles each `.c` file with the
current `xcc` binary and diffs the output against `.expected`.

To run all tests:

```sh
make test
```

To regenerate baselines after an intentional codegen change:

```sh
GENERATE=1 bash tests/run_tests.sh ./build/bin/xcc
```

Review the diff before committing regenerated baselines.

Current count: **50 tests** (`t001_return_zero` – `t050_static`).

---

## Planned Test Layers

The following layers are described in `docs/SIMPLIFICATION.md` (Stage 6)
and are not yet implemented.  Plain-text snapshot tests are preferred over
custom harnesses.

### Parser tests

What to cover:
- AST shape snapshots for focused constructs (expressions, declarations,
  control flow, initializers, edge cases)
- Diagnostic snapshots for malformed input (expected errors and warnings)

Suggested location: `tests/parser/`

### Preprocessor tests

What to cover:
- Macro expansion (object-like, function-like, variadic)
- `#include` resolution
- `#if` / `#ifdef` / `#elif` chains
- `#error` and diagnostics

Suggested location: `tests/preproc/`

### IR tests

What to cover:
- Textual IR dumps for expressions, control flow, loops, initializer lowering
- Verify that IR optimizer passes produce the expected output for known inputs

Suggested location: `tests/ir/`

### Backend instruction-selection tests

What to cover:
- Per-IR-op assembly output snapshots (one test per `icode_op` handler)
- Operand addressing (local, global, TLS, pointer dereference)
- Call/return convention (argument passing, return value)

Suggested location: `tests/backend/`

### Peephole rule tests

What to cover:
- One input/output assembly snippet per rule
- Verify that each rule fires and produces exactly the expected replacement
- Verify that no rule fires when the pattern is absent

Suggested location: `tests/peephole/`

---

## Adding a New End-to-End Test

1. Write a C source file in `tests/data/core/tNNN_name.c`.
2. Run `GENERATE=1 bash tests/run_tests.sh ./build/bin/xcc` to produce the
   baseline `.expected` file.
3. Review the generated assembly to confirm it is correct.
4. Commit both files together.

Do not commit a `.expected` file produced by a compiler build that has
unresolved bugs or deliberate output changes under review.
