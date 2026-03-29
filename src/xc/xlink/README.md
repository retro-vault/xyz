# XLINK - SDCC-style Z80 Linker for XYZ

`xlink` is a custom linker for the XYZ toolchain.

It reads SDCC-style `.rel` modules, resolves symbols (including selective library pulls), places areas in memory, applies relocations, and emits a relocatable executable image with an embedded relocation table.

## What It Does

- Parses SDCC-style textual `.rel` records used by this project (`X/H/M/O/A/S/T/R`).
- Supports area flags:
  - `REL/ABS` areas
  - `CON/OVR` placement behavior
- Resolves global symbols across modules and reports:
  - duplicate definitions
  - unresolved references
- Supports library resolution from `.lib` inputs:
  - scans candidate modules for definitions
  - pulls in only modules that satisfy currently undefined symbols
  - repeats until fixpoint
- Supports reserved memory holes via repeatable `-r start-end`:
  - area placement skips these ranges
  - useful for protecting register pages, ROM windows, or IF1/microdrive-sensitive regions
- Applies relocations (`word`, `symbol/area`, `pc-relative`, `msb` modes).
- Emits a custom relocatable binary format:
  - 12-byte header
  - relocation table (`offset,size,pad` entries)
  - relocated code/data payload

## CLI

```bash
xlink [options] <file.rel|file.lib> ...

options:
  -o <file>         output file (default: a.out)
  -e <symbol>       entry point symbol (default: _main)
  -r <start>-<end>  reserve address range (hex), repeatable
  -m                print memory map after linking
  -v                verbose output
  -h, --help        show help
```

Example:

```bash
xlink -o prog.xl -e _main \
  -r 0000-003F -r 3D00-3DFF \
  app.rel drivers.rel runtime.lib
```

## Output File Layout

Header (12 bytes, little-endian fields):

- `0x58 0x4C` magic (`'X' 'L'`)
- `u8 version` (currently `0x01`)
- `u8 flags` (currently `0x00`)
- `u16 entry_point`
- `u16 code_size`
- `u16 reloc_count`
- `u16 reserved` (currently `0x0000`)

Then:

- relocation table: `reloc_count` entries, each 4 bytes:
  - `u16 offset`
  - `u8 size` (`1` or `2`)
  - `u8 pad` (`0`)
- code/data bytes (`code_size` bytes)

## Build and Test

```bash
make
make test
```

Outputs:

- linker binary: `bin/xc/xlink/xlink`
- test binary: `build/xc/xlink/xlink_tests`

## Current Compatibility Notes

`xlink` is designed around SDCC-like object semantics, with a practical subset implemented today:

- `.rel`: textual SDCC record parsing as used in project fixtures and generated modules.
- `.lib`: currently parsed as a text index file with one `.rel` path per line (resolved relative to the `.lib` file location).

Also, for cross-module symbol placement, the current relocation logic assumes symbol values are based on the defining module's first area (`areas()[0]`). This matches common `_CODE`-first layouts in this project, but should be kept in mind for unusual area ordering.
