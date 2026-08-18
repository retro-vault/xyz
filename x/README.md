# x

X is the standalone Z80 cross-development toolchain: compiler, assembler,
linker, optimizer, archiver, object converter, debugger, emulator, hand-written
assembly libc/runtime, and selectable target platforms.

## Layout

- `src/` — tool executables (`xcc`, `xas`, `xld`, `xgdb`, `xemu`, `xar`, `xobjcopy`, `xprog`, `xopt`)
- `lib/` — host-side implementation libraries (`xbfd`, `rsp`, `xgdb`, `xemu`, `xopt`, `xz80`)
- `runtime/` — target runtime helper routines copied from the compiler tree
- `libc/` — target C library
- `targets/` — new home for extracted CPU and ABI definitions
- `platforms/` — target platform backends and platform contract headers
- `pkg/` — copied distribution-side packaging pieces such as `xgdb-vsix`
- `tests/` — shared test tools plus canonical suite roots under `tests/tests/`
- `examples/` — small programs demonstrating staged target platforms
- `docs/` — copied repository docs plus the `xcc` internals docs

## Build

```sh
make -C x
```

The relocatable toolchain prefix is staged under `bin/x/`; target headers and
libraries are under `bin/x/z80/`. The ordinary build defaults to the medium
`M` library model; use `X_MODEL=S` or `X_MODEL=L` to select another model, or
the root `x-s`, `x-m`, and `x-l` targets to stage all three side by side.
`make test-x-models` validates each model against its declared surface. The
unfiltered end-to-end runner uses the side-by-side `bin/x-l` compiler so that
double and long-long tests remain part of exhaustive validation without
changing the ordinary medium-model default.

## ZX Spectrum 48K quick start

```sh
mkdir -p build/examples/zx-ram build/examples/zx-rom

bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary \
  x/examples/zx-ram/lorem.c -o build/examples/zx-ram/lorem.bin
bin/x/bin/xprog --tap build/examples/zx-ram/lorem.bin \
  -o build/examples/zx-ram/lorem.tap --name LOREM

bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary \
  x/examples/zx-rom/lorem.c -o build/examples/zx-rom/lorem.rom
```

`zx-ram` starts at `0x5CCB`, and `zx-rom` produces an exact 16 KiB replacement
ROM. Both use an assembly keyboard/console backend, expose non-blocking
`kbhit()` through `<conio.h>`, and render the proportional Tamsyn
font. See the [complete ZX Spectrum guide](docs/howtos/ZX-SPECTRUM-48K.md) and
the separate [`zx-ram`](examples/zx-ram/README.md) and
[`zx-rom`](examples/zx-rom/README.md) examples.

## Notes

- `tests/tests/` is now the canonical home for non-benchmark test suites.
- Benchmarks now live under the unified `tests/benchmarks/` root.
- The pinned seven-lane z88dk comparison is correct on 24/24 for XCC M `-Os`
  and `-Of`; `-Of` is strictly fastest on 13/24 programs against the best
  valid current zsdcc/80cc result. See
  [`tests/benchmarks/z88dk24/RESULTS.md`](tests/benchmarks/z88dk24/RESULTS.md).
- `tests/tests/corpus/upstream/` holds the upstream corpora that previously lived under the repo-level `orig/`.
- For a Docker-based MinGW host-tools preflight, run `make -C x windows-host-preflight`.
- The optional ZX Spectrum MCP regression executes raw RAM, replacement ROM,
  TAP, and TZX forms; see `tests/tests/zx48/README.md`.
