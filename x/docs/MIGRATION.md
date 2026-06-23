# x Migration Map

This directory is the first copied product view for the X tools.

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

- build-root rewiring so the copied tree becomes the primary build layout
- extraction of clean CPU and ABI data into `targets/`
- further pruning and reorganization of the mixed copied test suites
