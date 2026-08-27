# xcc — C compiler

C23-oriented compiler driver for the Z80. Works like the GNU C driver: it
compiles C, calls `xas` to assemble, and calls `xld` to link.

## Synopsis

```bash
xcc [options] <input>... [-o <output>]
```

## Common usage

```bash
# Compile, assemble, and link in one step
xcc hello.c -o hello.xl

# Several translation units, mixed input kinds
xcc main.c util.s extra.rel -o app.xl

# Compile and assemble only, emit hello.rel
xcc -c hello.c

# Compile only, emit hello.s (use "-o -" for stdout)
xcc -S hello.c

# Compile preprocessed stdin directly to assembly (SDCC c1-mode)
cpp -Iinclude -DVALUE=1 hello.c | xcc --c1mode -mz80 --opt-code-size -o hello.s

# Optimized release build
xcc -Os main.c util.c -o app.xl

# Produce a flat binary at a fixed address
xcc main.c --oformat=binary -Ttext=0x8000 -o app.bin

# ZX Spectrum RAM program and replacement ROM
xcc -Os --platform=zx-ram --oformat=binary main.c -o app.bin
xcc -Os --platform=zx-rom --oformat=binary main.c -o app.rom

# Amstrad CPC firmware program
xcc -Os --platform=cpc-464 --oformat=binary main.c -o app.bin
xcc -Os --platform=cpc-6128 --oformat=binary main.c -o app.bin

# Build with debug info for xgdb
xcc -g main.c -o app.xl
```

## Options

| Option | Meaning |
|---|---|
| `-o <file>` | Output file |
| `-c` | Compile and assemble only, emit `.rel` |
| `-S` | Compile only, emit assembly |
| `--c1mode`, `-c1-mode` | Read preprocessed C from stdin, skip preprocessing, and emit assembly (`stdout` by default) |
| `-O0/-O1/-O2/-O3/-Os/-Of` | Optimization level (default `-O0`) |
| `--opt-code-size`, `--opt-code-speed` | SDCC-compatible aliases for `-Os` and `-Of` |
| `-f<name>`, `-fno-<name>` | Enable/disable a single optimization family |
| `-w`, `-W0..-W3`, `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror[=<name>]`, `-Wno-error[=<name>]` | Driver warning controls |
| `-mz80` | Accepted as a no-op for SDCC/z88dk compatibility |
| `-I<dir>` | Add include directory |
| `-D<macro>[=val]` | Define preprocessor macro |
| `--nostdinc` | Do not add xcc's default target include directory |
| `-g` | Emit debug info (for `xgdb`) |
| `--platform <name>`, `--platform=<name>` | Select target platform (default `none`) |
| `--float-format <fmt>`, `--float-format=<fmt>` | Select the ABI used for C `float`: `ieee32`, `ieee16`, `fixed8_8`, `fixed16_16`, or `fixed24_8` |
| `-masm <dialect>`, `-masm=<dialect>` | Assembler dialect: `sdasz80` (default) or `gnuas` |
| `--sdcccall <0\|1>` | Select the default SDCC-compatible calling convention |
| `--dump-ir` | Dump lowered IR to stderr |
| `--mode=sdcc`, `--mode=gnu` | Select the assembler output dialect |
| `-L<dir>`, `-l<name>`, `-B <prefix>` | Forwarded to the linker |
| `-nostdlib`, `-nostartfiles`, `--no-default-runtime` | Forwarded to the linker |
| `--oformat=<fmt>`, `-T*`, `--script=<file>`, `--section-start=<name>=<addr>`, `--binary-range=<lo>-<hi>`, `--reserve=<lo>-<hi>`, `-e <sym>`, `-Map=<file>`, `-M` | Forwarded to the linker (`xl`, `binary`, and `ihx` are active; primary `elf` output is still reserved) |
| `-Wl,<args>` | Forward comma-separated arguments to the linker |
| `-v` | Verbose: print the xas/xld commands being run |
| `--version` | Print version and exit |
| `-h`, `--help` | Show usage and exit |

## Defaults

The compiler finds its headers and runtime relative to its own install
location: common headers in `<prefix>/z80/include`, selected-platform headers
in `<prefix>/z80/include/<platform>`, and runtime and libraries in
`<prefix>/z80/lib`. Platform headers are searched before common headers. No
environment variables or wrapper scripts are needed; the prefix can be copied
anywhere.

The staged platform names are `none`, `cpc-464`, `cpc-664`, `cpc-6128`,
`cpm3`, `emu`, `zx-ram`, and `zx-rom`.
The ZX RAM platform links at `0x5CCB`; the ZX ROM platform emits a fixed
16 KiB replacement ROM. See `ZX48.md` and package RAM binaries with
`xprog --tap` or `xprog --tzx`.

The CPC targets link at `0x4000`. See `CPC.md`; package 464 binaries with
`xprog --cdt` and 664/6128 binaries with `xprog --dsk`.

## Float formats

`float` normally uses the IEEE-754 single-precision software runtime. For
small Z80 systems, `--float-format=` can change the C `float` ABI while
source code keeps using normal `float` variables, literals, operators,
arguments, and return values.

```bash
xcc --float-format=ieee32 main.c -o app-ieee.xl
xcc --float-format=ieee16 main.c -o app-half.xl
xcc --float-format=fixed8_8 main.c -o app-8_8.xl
xcc --float-format=fixed16_16 main.c -o app-16_16.xl
xcc --float-format=fixed24_8 main.c -o app-24_8.xl
```

Fixed formats automatically link the fixed-point runtime library.
When `<math.h>` is included, supported float-suffixed math functions such
as `fabsf`, `sqrtf`, `ceilf`, `floorf`, `truncf`, `roundf`, `fminf`,
`fmaxf`, `fdimf`, `copysignf`, `fpclassify`, `signbit`, `isfinite`,
`isinf`, and `isnan` are redirected to fixed-point helpers for the
selected format.

## C1 Mode

`--c1mode` is compatible with SDCC's "c1 mode": `xcc` reads already
preprocessed C from `stdin`, skips its own preprocessor, and emits
assembly. This is useful when a build system wants full control over
preprocessing while still using `xcc` for parsing, optimization, and Z80
code generation.

```bash
cpp -Iinclude -DVALUE=1 hello.c | xcc --c1mode -o hello.s
cpp -Iinclude -DVALUE=1 hello.c | xcc --c1mode > hello.s
```
