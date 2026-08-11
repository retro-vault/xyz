# x

First-pass product copy for the X tools.

## Layout

- `src/` — tool executables (`xcc`, `xas`, `xld`, `xgdb`, `xemu`, `xar`, `xobjcopy`, `xprog`, `xopt`)
- `lib/` — host-side implementation libraries (`xbfd`, `rsp`, `xgdb`, `xemu`, `xopt`, `xz80`)
- `runtime/` — target runtime helper routines copied from the compiler tree
- `libc/` — target C library
- `targets/` — new home for extracted CPU and ABI definitions
- `platforms/` — target platform backends and platform contract headers
- `pkg/` — copied distribution-side packaging pieces such as `xgdb-vsix`
- `tests/` — shared test tools plus canonical suite roots under `tests/tests/`
- `docs/` — copied repository docs plus the `xcc` internals docs

## Notes

- `tests/tests/` is now the canonical home for non-benchmark test suites.
- Benchmarks now live under the unified `tests/benchmarks/` root.
- `tests/tests/corpus/upstream/` holds the upstream corpora that previously lived under the repo-level `orig/`.
- For a Docker-based MinGW host-tools preflight, run `make -C x windows-host-preflight`.
