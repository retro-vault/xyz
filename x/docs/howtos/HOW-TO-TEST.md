# How to Test

This guide reflects the active X product layout. Run commands from the
repository root unless a section says otherwise.

## Build before testing

```sh
make -C x
```

That builds the host tools and stages the target headers, runtime, libc, CRTs,
linker scripts, and platform archives under `bin/x/`.

## Unified compiler and libc suite

The canonical manifest-driven suite uses `xemutest` and `xemu`:

```sh
bash x/tests/run_tests.sh --filter xcc
```

The runner builds itself automatically and uses `bin/x/bin/xcc` by default.
An explicit compiler may be supplied as the first argument:

```sh
bash x/tests/run_tests.sh /path/to/xcc --filter xcc
```

Useful selection switches are:

```sh
bash x/tests/run_tests.sh --list
bash x/tests/run_tests.sh --filter xcc_exec_
bash x/tests/run_tests.sh --filter stdio
bash x/tests/run_tests.sh --kind compile
bash x/tests/run_tests.sh --kind run --abi 1
bash x/tests/run_tests.sh --filter xcc --verbose
```

Filters match test IDs, aliases, tags, paths, and component names. The active
manifests live under `x/tests/tests/c23/`; see
[`x/tests/tests/README.md`](../../tests/tests/README.md) for their layout.

## Direct assembly-library tests

The libc suite links the hand-written assembly directly into emulator images.
It is split to keep executable size and symbol maps manageable:

```sh
make -C x/tests/tests/libc core-test
make -C x/tests/tests/libc scan-test
make -C x/tests/tests/libc wide-test
make -C x/tests/tests/libc test
```

Run the compiler runtime tests separately:

```sh
make -C x/tests/tests/runtime test
```

These are the fastest checks for a libc/runtime implementation change because
they call target symbols through `runtime_machine` without requiring a full C
front-end regression for every case.

## Host-tool component tests

Use the owning component's target for focused changes:

```sh
make -C x/src/xas test
make -C x/src/xas test-libs
make -C x/src/xcc test
make -C x/src/xcc test-all
make -C x/src/xemu test
make -C x/src/xgdb test
make -C x/src/xld test
make -C x/src/xobjcopy test
make -C x/src/xopt test
make -C x/src/xprog test

make -C x/lib/xemu test
make -C x/lib/xgdb test
make -C x/lib/xz80 test
```

`xas test-libs` is the heavier archive/library round-trip parity sweep. The
other component tests cover their parsers, emitters, CLI behavior, and focused
integration paths.

## Package verification

Build both optional X package artifacts through the public entry point:

```sh
make packages
```

This builds the Debian toolchain package and XGDB VSIX. The Debian target then
extracts the finished `.deb` and verifies required tools, manuals, target
headers/libraries, each installed platform's CRT source/object and linker
scripts, normalized ownership/modes, and the CPC `xprog --cdt`/`--dsk` modes.
Repeat the archive check without rebuilding the wider toolchain with:

```sh
make -C x/pkg/debian check
```

For CPC release acceptance, extract the `.deb` and invoke the CPC MCP runner
with the package's `/opt/x/bin/xcc` and `/opt/x/bin/xprog`; this proves the
installed sysroot rather than the checkout prefix.

## CP/M integration test

The CP/M COM regression uses `tnylpo` to execute the staged `cpm3` backend:

```sh
make -C x/tests/tests/hello-cpm test TNYLPO=/path/to/tnylpo
```

Besides startup, console output, and the complete command-tail matrix, it
checks `<stdio.h>` `trygetchar()` with an open idle console and with a waiting
character. The ready test then calls `getchar()` to prove that the status call
did not consume the character.

## ZX Spectrum hardware-level test

The optional ZX test uses
[zx-spectrum-mcp](https://github.com/retro-vault/zx-spectrum-mcp) and a legal
48K ROM image to run every delivery form:

```sh
python3 x/tests/tests/zx48/run_mcp.py \
  --mcp /path/to/zx-spectrum-mcp \
  --rom /path/to/48.rom
```

It compiles one stdlib smoke program as raw `zx-ram` and `zx-rom`, packages the
RAM binary as TAP and TZX, types `LOAD ""` into the real ROM for tape cases,
plays the waveform, checks the public non-blocking keyboard poller as well as
normal/CAPS/SYMBOL blocking input, and verifies a `0xA5` memory marker. Success
requires all four lines:

```text
PASS RAM binary (...=0xA5)
PASS replacement ROM (...=0xA5)
PASS TAP (...=0xA5)
PASS TZX (...=0xA5)
```

See the [ZX Spectrum target guide](ZX-SPECTRUM-48K.md) for the visual Fuse
example and platform contract.

## Amstrad CPC hardware-level test

The optional CPC test uses `amstrad-cpc-mcp` plus legal 464, 664, 6128, and
AMSDOS ROM images:

```sh
python3 x/tests/tests/cpc/run_mcp.py \
  --mcp /path/to/amstrad-cpc-mcp \
  --roms /path/to/rom-directory
```

It compiles the shared smoke program for all three CPC targets, creates a CDT
for the 464 and writable DSK images for the 664 and 6128, boots them through
the real firmware, and checks libc, console input, the firmware clock, and
clean startup/exit. The disk models additionally exercise raw and stdio file
I/O, seeks in headerless files, rename/remove, and missing-file errors. See
the [Amstrad CPC target guide](AMSTRAD-CPC.md) for the platform and media
contracts.

## Codegen Benchmarks

For a repeatable compiler-size benchmark against SDCC that measures only
generated translation-unit code, use:

```sh
bash archive/x/tests/tests/xcc-legacy/run_codegen_bench.sh ./bin/x/bin/xcc
```

By default this benchmarks the `exec/int` suite with `sdcccall(1)` and
records `xcc -O0/-O1/-O2/-Of/-O3/-Os` against SDCC `--opt-code-size`
and `--opt-code-speed`. If SDCC cannot compile an xcc-specific probe,
that row is recorded as `n/a` for SDCC and kept in the xcc totals.

Outputs land under:

```sh
build/xc/xcc/bench/codegen/int/
```

The benchmark writes:

- `results.csv` with per-test byte counts
- `summary.md` with totals, deltas, `-Os` versus `-O2`, and `-O3` versus
  `-Os` comparisons
- `versions.txt` with the exact tool versions used

To cover all exec suites instead of only integer tests:

```sh
bash archive/x/tests/tests/xcc-legacy/run_codegen_bench.sh ./bin/x/bin/xcc --suite all
```

## Practical change checklist

1. Make the code change.
2. Run the smallest direct test that covers the touched area.
3. Run `bash x/tests/run_tests.sh --filter xcc` for compiler/libc-facing work.
4. Run the affected host-tool component test for tool changes.
5. Run the four-mode ZX MCP test for Spectrum CRT, linker, console, keyboard,
   or tape changes.
6. Run the three-model CPC MCP test for CPC CRT, firmware, AMSDOS, CDT, or DSK
   changes.
7. If packaging inputs or staged outputs changed, run `make packages` and its
   finished-archive check.

## Prerequisites and Skip Behavior

- `sdasz80` must be on `PATH` for assembler parity checks.

- `make -C x/src/xas test-libs` is a heavier library round-trip parity sweep.
  It also needs the GNU Z80 assembler (`${Z80_GNU_PREFIX}as` by default).

- Execution-oriented XCC cases need:
  `sdasz80`, `sdldz80`, `g++`, and the configured Z80 GNU binutils
  (`z80-unknown-elf-as`, `z80-unknown-elf-ld`, `z80-unknown-elf-objcopy`
  by default).

- `python3` is required by the ZX MCP runner and several test generators.

- The ZX test additionally needs the `zx-spectrum-mcp` executable and a 16 KiB
  standard 48K ROM path. The ROM is not bundled with X.
