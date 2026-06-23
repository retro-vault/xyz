# xobjcopy

`xobjcopy` is the xyz toolchain's object/archive copy and conversion tool for Z80.
It is intentionally modeled after GNU `objcopy`, but it delegates format support to
`libxbfd` instead of hardcoding object formats in the tool itself.

## Supported input/output kinds

- SDCC REL objects: `rel`, `sdcc-rel`
- GNU ELF32 Z80 objects: `elf`, `elf32-z80`, `elf32-littlez80`
- text-index archives: `lib`, `text-archive`
- GNU ar archives: `a`, `ar`, `gnu-ar`, `binary-archive`

## Examples

Convert an SDCC REL object into ELF:

```sh
xobjcopy -I rel -O elf foo.rel foo.o
```

Strip inline debug sections from an ELF object:

```sh
xobjcopy --strip-debug foo.o stripped.o
```

Convert a text archive into a GNU `ar` archive:

```sh
xobjcopy -I lib -O a libstuff.lib libstuff.a
```

## Current limitations

- `--strip-debug` currently supports object inputs only.
- Raw binary / Intel HEX conversions are not implemented in this first version.
