# Active C23 Cases

This directory holds the manifest-driven `xemutest` cases used by
`x/tests/run_tests.sh`.

## Layout

- `probes/` — small hand-authored smoke and debugger checks
- `xcc/core/` — compile-only language and frontend cases
- `xcc/sema/` — diagnostic and warning behavior cases
- `xcc/exec/` — executable end-to-end cases grouped by value domain
- `xcc/external/` — imported standards papers, DRs, and tsuite coverage

Each leaf case directory contains a `test.cfg` plus any local fixture files
needed only by that case.
