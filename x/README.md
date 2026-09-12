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
- `pkg/` — Debian toolchain and XGDB VSIX packaging, with archive verification
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

## Packages

Build the optional installable artifacts from the repository root:

```sh
make packages
```

This stages the XGDB VSIX under `bin/x/pkg/vsix/` and builds the native X
toolchain package under `bin/x/pkg/deb/`. The Debian build extracts its finished
archive and verifies the host tools, documentation, common target libraries,
and every packaged platform's CRT source/object, GNU/SDCC linker scripts, and
platform archive. Run `make -C x/pkg/debian check` to repeat that archive check.
See [`pkg/README.md`](pkg/README.md) for the complete contract.

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
`trygetchar()` through `<stdio.h>`, and render the proportional Tamsyn
font. See the [complete ZX Spectrum guide](docs/howtos/ZX-SPECTRUM-48K.md) and
the separate [`zx-ram`](examples/zx-ram/README.md) and
[`zx-rom`](examples/zx-rom/README.md) examples.

For divIDE with esxDOS firmware, select `--platform=zx-esxdos`. This separate
48K RAM target starts at `0x8000` and provides disk-backed file descriptors and
stdio. See the [esxDOS guide](docs/dist/man/ZX-ESXDOS.md) and
[disk example](examples/zx-esxdos/README.md). For an application that boots
from a replacement ROM without Sinclair BASIC, use `--platform=zx-esxdos-rom`;
see the [ROM guide](docs/dist/man/ZX-ESXDOS-ROM.md) and
[ROM disk example](examples/zx-esxdos-rom/README.md).

## Amstrad CPC quick start

```sh
bin/x/bin/xcc -Os --platform=cpc-464 --oformat=binary \
  x/examples/cpc-464/hello.c -o build/examples/cpc-464/hello.bin
bin/x/bin/xprog --cdt build/examples/cpc-464/hello.bin \
  --name HELLO -o build/examples/cpc-464/hello.cdt

bin/x/bin/xcc -Os --platform=cpc-6128 --oformat=binary \
  x/examples/cpc-6128/files.c -o build/examples/cpc-6128/files.bin
bin/x/bin/xprog --dsk build/examples/cpc-6128/files.bin \
  --name FILES.BIN -o build/examples/cpc-6128/files.dsk
```

The CPC 464 target is cassette-only and omits AMSDOS state. The CPC 664 and
6128 targets provide ROM-backed raw and stdio file operations through their
single input/output disk channels. See the
[complete Amstrad CPC guide](docs/howtos/AMSTRAD-CPC.md).

## YOS quick start

```sh
mkdir -p build/examples/yos
bin/x/bin/xcc -Os --platform=yos x/examples/yos/hello.c \
  -o build/examples/yos/hello.xl
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 1 \
  build/examples/yos/hello.xl -o build/examples/yos/hello.sys
bin/x/bin/xprog --esxdos --name HELLO.SYS build/examples/yos/hello.sys \
  -o build/examples/yos/hello.ide
```

The backend produces relocatable XL code, obtains ABI 1 through
`query_service("yos")`, and routes allocation and POSIX files through YOS.
Standard console output is silent unless the process installs the
`yos_set_putchar_hook()` extension. See the [YOS example](examples/yos/) and
the [Programming YOS book](../y/docs/books/PROGRAMMING-YOS.md).

## Notes

- `tests/tests/` is now the canonical home for non-benchmark test suites.
- Benchmarks now live under the unified `tests/benchmarks/` root.
- The pinned current-upstream seven-lane z88dk comparison is correct on 24/24
  for XCC M `-Os` and `-Of`; with 80cc as the primary competitor, `-Os` is
  strictly smallest on 24/24 and `-Of` strictly fastest on 24/24. The same
  specialized profiles win 24/24 against the broader valid SDCC/80cc
  envelope. See
  [`tests/benchmarks/z88dk24/CURRENT-RESULTS.md`](tests/benchmarks/z88dk24/CURRENT-RESULTS.md);
  the separate historical hybrid remains in
  [`RESULTS.md`](tests/benchmarks/z88dk24/RESULTS.md).
- `tests/tests/corpus/upstream/` holds the upstream corpora that previously lived under the repo-level `orig/`.
- For a Docker-based MinGW host-tools preflight, run `make -C x windows-host-preflight`.
- The optional ZX Spectrum MCP regression executes raw RAM, replacement ROM,
  TAP, and TZX forms; see `tests/tests/zx48/README.md`.
- The explicit Amstrad CPC MCP release validation boots a generated CDT on a
  464 and generated DSK images on a 664 and 6128. It is outside the regression
  pack; run
  `python3 x/tests/tests/cpc/run_mcp.py`.
