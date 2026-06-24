# Unified X Tests

This directory is the active non-benchmark test surface for `x`.

## Main Areas

- `c23/` — the unified compiler-facing test tree used by `x/tests/run_tests.sh`
- `libc/`, `runtime/`, `xas/`, `xemu/`, `xgdb/`, `xz80/`, ... — component-owned tests and helpers
- `archive/` at the repo root — legacy suite manifests and harness entrypoints that are no longer part of the default unified run

## Unified C23 Layout

The active compiler suite now lives entirely under `c23/`:

- `c23/cases/` — manifest-driven `xemutest` cases
- `c23/cases/probes/` — small hand-authored tool/probe cases
- `c23/cases/xcc/` — generated `xcc` compile/run cases grouped by area
- `c23/xcc/data/` — shared source payloads reused by generated `xcc` cases
- `c23/corpus/` — imported corpus projects and helpers
- `c23/tests/` / `c23/setups/` / `c23/docs/` — the imported compatibility matrix suite

## Active Metadata

Every active `xemutest` case uses `test.cfg` metadata with:

- `id` — required stable identifier
- `component` — owning area such as `xcc`
- `summary` — short human-readable description
- optional `alias` / `legacy_path`
- `runner = xemu` for manifest-driven compile/run cases

Common optional execution fields now include:

- `matrix_opt = O1|O2|O3`
- `matrix_float = ieee32|fixed8_8|fixed16_16|fixed24_8`
- `host_golden = gcc` for host-generated expected exit/stdout
- `timeout_seconds = N`
- `assert_var = name:type=value` for debug-symbol-backed variable checks

`xemutest --list` expands these matrices into the concrete runnable variants.
