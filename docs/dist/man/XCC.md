# xcc — C compiler

C11 compiler driver for the Z80. Works like the GNU C driver: it
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

# Optimized release build
xcc -Os main.c util.c -o app.xl

# Produce a flat binary at a fixed address
xcc main.c --oformat=binary -Ttext=0x8000 -o app.bin

# Build with debug info for xgdb
xcc -g main.c -o app.xl
```

## Options

| Option | Meaning |
|---|---|
| `-o <file>` | Output file |
| `-c` | Compile and assemble only, emit `.rel` |
| `-S` | Compile only, emit assembly |
| `-O0/-O1/-O2/-O3/-Os/-Of` | Optimization level (default `-O0`) |
| `-f<name>`, `-fno-<name>` | Enable/disable a single optimization family |
| `-I<dir>` | Add include directory |
| `-D<macro>[=val]` | Define preprocessor macro |
| `-g` | Emit debug info (for `xgdb`) |
| `--platform=<name>` | Select target platform (default `cpm3`) |
| `-masm=<dialect>` | Assembler dialect: `sdasz80` (default) or `gnuas` |
| `-L<dir>`, `-l<name>` | Forwarded to the linker |
| `-nostdlib`, `-nostartfiles` | Forwarded to the linker |
| `--oformat=<fmt>`, `-T*`, `-e <sym>` | Forwarded to the linker |
| `-Wl,<args>` | Forward comma-separated arguments to the linker |
| `-v` | Verbose: print the xas/xld commands being run |

## Defaults

The compiler finds its headers and runtime relative to its own install
location: headers in `<prefix>/z80/include`, runtime and libraries in
`<prefix>/z80/lib`. No environment variables or wrapper scripts are
needed; the prefix can be copied anywhere.
