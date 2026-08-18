# X migration record

`x/` is now the active, independently buildable product root. This file keeps
the old-to-current path mapping for archaeology; new documentation and commands
must use the current paths on the right.

## Current Mapping

- `src/xc/xcc/` -> `x/src/xcc/`
- `src/xc/xas/` -> `x/src/xas/`
- `src/xc/xld/` -> `x/src/xld/`
- `src/xc/xgdb/` -> `x/src/xgdb/`
- `src/xc/xar/` -> `x/src/xar/`
- `src/xc/xobjcopy/` -> `x/src/xobjcopy/`
- `src/xc/xopt/` -> `x/src/xopt/`
- `tools/xgdb-vsix/` -> `x/pkg/xgdb-vsix/`
- `lib/xbfd/` -> `x/lib/xbfd/`
- `lib/rsp/` -> `x/lib/rsp/`
- `lib/xgdb/` -> `x/lib/xgdb/`
- `lib/xopt/` -> `x/lib/xopt/`
- `lib/xz80/` -> `x/lib/xz80/`
- `src/xc/xcc/lib/runtime/` -> `x/runtime/`
- `lib/libc/` -> `x/libc/`
- `lib/sys/` -> `x/platforms/`
- selected repo tests -> `x/tests/`
- `orig/c23-corpus/` -> `x/tests/corpus/upstream/c23-corpus/`
- `orig/thealgorithms-c/` -> `x/tests/corpus/upstream/thealgorithms-c/`

## Still Pending

- extraction of clean CPU and ABI data into `targets/`
- further pruning and reorganization of the mixed copied test suites

## Completed product-boundary work

- `make -C x` builds and stages the standalone prefix under `bin/x/`.
- The root build delegates to X before building YOS with the staged tools.
- Host SDK, target headers, libc, runtime, CRTs, linker scripts, and named
  platform archives have prefix-rooted locations.
- CP/M 3, ZX Spectrum RAM, and ZX Spectrum replacement-ROM targets are staged
  platform selections rather than private OS build fragments.
- X and Y own their release notes, documentation, tests, and packaging roots.
