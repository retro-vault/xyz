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
| `--platform=<name>` | Select target platform library (default `cpm3`) |
| `-nostartfiles` | No implicit startup file |
| `-nostdlib` | No implicit startup file or default libraries |
| `--no-default-runtime` | Do not auto-probe the install prefix for runtime assets |
| `--oformat=xl\|binary\|elf\|ihx` | Output format (default `xl`) |
| `-T <file>`, `--script=<file>` | Linker script |
| `-Ttext/-Tdata/-Tbss=<addr>` | Section base addresses |
| `--section-start=<name>=<addr>` | Base address for a named section |
| `--binary-range=<lo>-<hi>` | Limit emitted range for binary output |
| `--reserve=<lo>-<hi>` | Reserve an address range (repeatable) |
| `-g` | Emit debug outputs (CDB) for use with xgdb |
| `-M`, `-Map=<file>` | Print/write memory map |

## Output formats

- **xl** (default) — relocatable XL image with header and relocation
  table, loadable at any address.
- **binary** — flat memory image.
- **ihx** — Intel HEX.
- **elf** — ELF image with DWARF debug sections (gnu mode).
