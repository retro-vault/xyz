# xobjcopy — object converter

Converts Z80 objects and archives between formats, and strips debug
metadata.

## Synopsis

```bash
xobjcopy [options] <input> [output]
```

## Common usage

```bash
# Convert an SDCC .rel object to ELF32
xobjcopy -I rel -O elf main.rel main.o

# Convert an ELF object back to .rel
xobjcopy -I elf -O rel main.o main.rel

# Repack a text-index archive as GNU ar
xobjcopy -I lib -O ar libfoo.lib libfoo.a

# Strip debug metadata
xobjcopy --strip-debug main.rel main-stripped.rel
```

## Options

| Option | Meaning |
|---|---|
| `-I <target>`, `--input-target=` | Input format |
| `-O <target>`, `--output-target=` | Output format |
| `-o <file>` | Output file |
| `-g`, `--strip-debug` | Remove inline debug sections/metadata |

## Supported targets

| Name | Format |
|---|---|
| `rel`, `sdcc-rel` | SDCC `.rel` object |
| `elf`, `elf32-z80` | GNU ELF32 Z80 object |
| `lib`, `text-archive` | Text-index archive |
| `a`, `ar`, `gnu-ar` | GNU ar archive |

Formats are detected automatically when `-I`/`-O` are omitted.
