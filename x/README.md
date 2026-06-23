# x

First-pass product copy for the X tools.

## Layout

- `src/` — tool executables (`xcc`, `xas`, `xld`, `xgdb`, `xar`, `xobjcopy`, `xopt`)
- `lib/` — host-side implementation libraries (`xbfd`, `rsp`, `xgdb`, `xopt`, `xz80`)
- `runtime/` — target runtime helper routines copied from the compiler tree
- `libc/` — target C library
- `targets/` — new home for extracted CPU and ABI definitions
- `platforms/` — target platform backends and platform contract headers
- `pkg/` — copied distribution-side packaging pieces such as `xgdb-vsix`
- `tests/` — copied tool, libc, runtime, corpus, benchmark, and compatibility suites
- `docs/` — copied repository docs plus the `xcc` internals docs

## Notes

- This is an additive migration step. The legacy top-level layout still exists for now.
- The canonical tool-owned tests are still the copies under `src/*/tests/`; the top-level `tests/` tree is currently an aggregated migration copy.
- `tests/corpus/upstream/` now holds the upstream corpora that previously lived under the repo-level `orig/`.
- The source copy is in place, but not every build script has been fully rewired to the new layout yet.
