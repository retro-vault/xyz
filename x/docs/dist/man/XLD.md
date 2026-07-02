# xld — linker

Z80 linker. Links SDCC/ASxxxx `.rel` objects and `.lib`/`.a` archives
(or GNU ELF objects in `--mode=gnu`) into an executable image.

## Synopsis

```bash
xld [options] <input>...
```

Unless told otherwise, xld automatically links the startup file
(`crt0.rel`), the C library, the runtime library, and the platform
library from `<prefix>/z80/lib`, so a plain `xld main.rel -o app.xl`
produces a complete program.

## Common usage

```bash
# Link objects and a library into an XL image
xld main.rel util.rel libfoo.a -o app.xl

# Flat binary at a fixed address
xld --oformat=binary -Ttext=0x8000 main.rel -o app.bin

# Intel HEX output with a memory map
xld --oformat=ihx -Map=app.map main.rel -o app.ihx

# Bare-metal link: no implicit startup files or libraries
xld -nostdlib -e _start boot.rel -o boot.bin --oformat=binary

# Use a linker script
xld -T layout.ld main.rel -o app.xl
```

## Options

| Option | Meaning |
|---|---|
| `-o <file>` | Output file (default `a.out`) |
| `-e <symbol>` | Entry symbol (default `_main` in sdcc mode, `_start` in gnu mode) |
| `--mode=sdcc` / `--mode=gnu` | Input flavor (default `sdcc`) |
| `-L<dir>`, `-l<name>` | Library search directory / library |
| `-B <prefix>`, `--sdcc-runtime <dir>` | Add the startup/runtime/toolchain search prefix |
| `--platform <name>`, `--platform=<name>` | Select target platform library (default `none`) |
| `-nostartfiles` | No implicit startup file |
| `-nostdlib` | No implicit startup file or default libraries |
| `--no-default-runtime` | Do not auto-probe the install prefix for runtime assets |
| `-f <fmt>`, `--oformat=xl\|binary\|ihx` | Output format (default `xl`) |
| `--oformat=elf` | Reserved; not yet implemented as a primary output format |
| `-T <file>`, `--script <file>`, `--script=<file>` | Linker script |
| `-b <name>=<addr>`, `--section-start=<name>=<addr>` | Base address for a named section |
| `-Ttext/-Tdata/-Tbss=<addr>` | Section base aliases |
| `-x <lo>-<hi>`, `--binary-range=<lo>-<hi>` | Limit emitted range for binary output |
| `-r <lo>-<hi>`, `--reserve=<lo>-<hi>` | Reserve an address range (repeatable) |
| `-g` | Emit debug outputs (CDB) for use with xgdb |
| `-m`, `-M`, `--print-map` | Print memory map |
| `-Map <file>`, `-Map=<file>` | Write memory map |
| `-v`, `--verbose` | Verbose output |
| `--version` | Print version and exit |
| `-h`, `--help` | Show usage and exit |

## Output formats

- **xl** (default) — relocatable XL image with header and relocation
  table, loadable at any address.
- **binary** — flat memory image.
- **ihx** — Intel HEX.
- **gnu debug sidecar** — in `--mode=gnu`, `-g` derives an ELF + DWARF
  debug sidecar next to the primary output.
