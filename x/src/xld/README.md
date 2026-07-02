# xld — Z80 Relocatable Linker

`xld` is the XYZ toolchain linker for Z80 targets.

In plain words, it does four main jobs:

1. read relocatable object files and libraries from the supported input modes
2. resolve symbols and pull in only the library modules that are needed
3. place areas in memory while respecting base addresses and reserved holes
4. write one or more output files for loading, ROM building, symbols, and debugging

What `xld` can do today:

- link SDCC `.rel` objects and GNU ELF `.o` / `.obj` objects
- read library inputs as:
  - xld text-index `.lib` libraries
  - native `ar` archives such as SDCC `.lib` and GNU `.a`
- place relocatable, absolute, and overlay areas
- skip reserved address ranges during placement
- emit relocatable `XL` output
- emit flat absolute `BIN` output
- emit a linked SDCC `.cdb` debug file in `--mode=sdcc`
- emit a derived GNU ELF + DWARF2 `.elf` debug sidecar in `--mode=gnu`
- optionally prepend a runtime `crt0` and append a runtime library from `--sdcc-runtime <dir>` in SDCC mode

Current limits that are worth knowing up front:

- there is no real banked-memory output format yet
- `OVR` is only shared-address placement inside one final image
- `OVR` does not produce separate overlay payload files
- if two overlaid modules write bytes to the same addresses, later copied bytes overwrite earlier ones in the final image

Current CLI note:

- The authoritative end-user switch summary is `x/docs/dist/man/XLD.md`
  together with `xld --help`.
- This README still contains deeper implementation notes and some historical
  discussion, but the packaged manpage is the current CLI contract.
- Historical sections below that mention removed outputs such as NoICE
  `.noi` files or older `.xgdb` sidecars should be read as background,
  not as the current CLI surface.

---

## What Files xld Reads

| File | Purpose | Required |
|------|---------|----------|
| `.rel` | SDCC/ASxxxx relocatable object. Contains areas, symbols, code bytes, and relocations. | Optional |
| `.o`, `.obj` | GNU ELF relocatable object. Uses section names such as `.text`, `.data`, and `.bss`. | Optional |
| `.lib` | Library input. Can be xld text-index format or a native `ar` archive. xld loads only members that satisfy unresolved symbols. | Optional |
| `.a` | GNU/SysV `ar` archive, typically containing ELF `.o` members. | Optional |
| `.adb` | SDCC C debug sidecar. Used as a fallback source for compiler records when emitting `.cdb` or GNU-mode debug sidecars. | Optional |
| `.cdb` | SDCC compiler debug sidecar. Preferred compiler-record source when emitting a linked `.cdb`. | Optional |
| `.lst` | Assembler listing. Used when emitting GNU/SDCC debug sidecars for assembly modules. | Optional |

### Notes

- `--mode=sdcc` defaults the entry symbol to `_main`, keeps `-Ttext/-Tdata/-Tbss` mapped to `_CODE/_DATA/_BSS`, and uses the SDCC-style runtime auto-probe rules.
- `--mode=gnu` defaults the entry symbol to `_start` and maps `-Ttext/-Tdata/-Tbss` to `.text/.data/.bss`.
- Both `.rel` and ELF `.o` eventually become the same internal module model inside `xld`.
- `.adb`, `.cdb`, and `.lst` are not linked themselves; they are sidecars used only to enrich debug outputs.
- `.adb` is usually present for SDCC C compilation with debug enabled.
- `.cdb` is usually present for SDCC C compilation with `--debug`.
- `.lst` is usually present for assembler output with listing/debug enabled.

### Linker scripts

`xld` now supports `-T <file>` and `--script=<file>`.

Scripts are parsed through `libxbfd` and loaded as defaults before normal
command-line processing. Any explicit command-line option still wins over the
script:

- `-e` overrides script `ENTRY`
- `-f` / `--oformat` override script output format
- `-x` overrides script binary range
- `-b`, `--section-start`, `-Ttext`, `-Tdata`, `-Tbss` override script bases
- `-r` / `--reserve` add extra reserved ranges on top of the script

#### GNU script subset

In `--mode=gnu`, `xld` accepts a practical GNU-ld style subset aimed at real
Z80 ROM/RAM scripts:

- `ENTRY(symbol)`
- `OUTPUT_FORMAT(binary|xl|elf|ihx)`
- `MEMORY { NAME : ORIGIN = ..., LENGTH = ... }`
- `SECTIONS { .text 0x0100 : { *(.text) } }`
- `SECTIONS { .text : { *(.text .text.*) *(.rodata .rodata.*) } > ROM }`
- output-section ordering derived from wildcard patterns inside `SECTIONS`
- `/DISCARD/ : { ... }`
- section attributes such as `AT(0x1234)` and `AT>ROM`
- simple assignments and assertions such as:
  - `_rom_end = .;`
  - `ASSERT(_rom_end <= 0x4000, "ROM overflow!")`

`xld` also understands one extra non-standard construct:

- `RESERVE(lo-hi)` or `RESERVE(lo, hi)`
- `BINARY_RANGE(lo-hi)` or `BINARY_RANGE(lo, hi)`

`RESERVE(...)` feeds the same reserved-hole machinery as `-r`.

#### SDCC script format

In `--mode=sdcc`, `xld` accepts either a simple keyword form:

```text
ENTRY _main
FORMAT bin
AREA _CODE = 0100
AREA _DATA = 4000
AREA _BSS  = 4100
RANGE 0000-7FFF
RESERVE 0100-017F
```

or the equivalent command-file style:

```text
-e _main
-f bin
-b _CODE=0100
-b _DATA=4000
-b _BSS=4100
-x 0000-7FFF
-r 0100-017F
```

It also tolerates real SDCC-style command files with one directive per line,
including harmless linker-driver commands such as `-p`, `-m`, `-z`, `-k`,
`-l`, and trailing object/library file lines before the final `-e`.

---

## What Files xld Writes

| File | How You Ask For It | What It Is |
|------|---------------------|------------|
| primary output, default `a.out` | always, via `-o <file>` | Relocatable `XL`, flat `BIN`, or Intel HEX `IHX`, depending on `-f` / `--oformat` |
| `.xl` | default `-f xl` | Relocatable XYZ loader image with header and relocation table |
| `.bin` | `-f bin` | Flat absolute binary image |
| `.ihx` | `-f ihx` | Intel HEX image |
| `.cdb` | derived by `-g` in `--mode=sdcc` | Linked SDCC CDB debug file |
| `.elf` | derived by `-g` in `--mode=gnu` | Derived ELF debug sidecar with DWARF2 sections |

### Which one should I use?

- Use `XL` when a loader will relocate the program at load time.
- Use `BIN` when you want a fixed-address ROM or raw memory image.
- Use `IHX` when you want Intel HEX records for PROM programmers or loaders.
- Use `-g` when you want source-level debugging metadata.
- In `--mode=sdcc`, `-g` derives a linked `.cdb`.
- In `--mode=gnu`, `-g` derives an ELF sidecar with DWARF2 sections.

---

## Quick Start

### 1. Produce a relocatable XL file

```bash
xld -o hello.xl \
      build/crt0.rel build/hello.rel
```

This writes relocatable `XL` output and uses `_main` as the entry symbol.

### 2. Produce a pure flat binary

```bash
xld -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

This links `_CODE` at `0x0100` and emits only the inclusive range
`0x0100..0x02FF` into `hello.bin`.

### 3. Produce an Intel HEX image

```bash
xld -f ihx -e _entry \
      -b _CODE=0100 \
      -o hello.ihx \
      build/crt0.rel build/hello.rel
```

This emits Intel HEX records instead of a flat binary image.

### 4. Produce a binary plus a linked SDCC CDB file

```bash
xld -f bin -g -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

In SDCC mode, `-g` derives `hello.cdb` next to `hello.bin`.

For the `.cdb` file to be rich:

- C modules should have sibling `.cdb` files from SDCC `--debug`
- if a sibling `.cdb` is missing, xld falls back to sibling `.adb`
- assembly modules should have sibling `.lst` files if you want `L:A...` line records

### 5. Produce a binary plus a GNU ELF + DWARF2 sidecar

```bash
xld --mode=gnu -f bin -g -e _start \
      -Ttext=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/start.o build/hello.o
```

In GNU mode, `-g` derives `hello.elf` next to `hello.bin`.

The ELF sidecar contains:

- a `.text` section derived from the final linked image
- a final symbol table
- `.debug_abbrev`, `.debug_info`, and `.debug_line` DWARF2 sections

### 6. Use an SDCC runtime directory only when you want it

```bash
xld --sdcc-runtime /path/to/runtime/z80 \
      -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/hello.rel
```

If `--sdcc-runtime` is not given, xld does not inject any startup object
or library on its own.

### 7. Reserve holes and print a memory map

```bash
xld -v -m -f bin -x 0000-3FFF -e _entry \
      -r 0000-003F \
      -r 1708-1708 \
      -b _CODE=0100 \
      -b _DATA=5B00 \
      -o prog.bin \
      build/crt0.rel build/main.rel build/runtime.lib
```

This:

- reserves the bottom 64 bytes
- reserves the single protected byte at `0x1708`
- pins `_CODE` to `0x0100`
- pins `_DATA` to `0x5B00`
- writes a flat ROM-style `BIN`
- if the emitted BIN window includes `0x1708`, keeps that byte `0x00` and, when possible, inserts a `JR` or `JP` immediately before it
- prints the final placement map

If you want the same placement rules but relocatable `XL` output, omit
`-f bin -x ...` and choose an `.xl` output path instead.

---

## Command-Line Usage

```text
xld [options] <input>...
```

Current commonly used switches:

| Option | Short meaning |
|--------|---------------|
| `--mode=sdcc`, `--mode=gnu` | Select the input/object flavor |
| `-g` | Write mode-specific debug output: `.cdb` in SDCC mode, `.elf` sidecar in GNU mode |
| `--sdcc-runtime <dir>` | Auto-inject runtime `crt0` and default library from a directory |
| `-o <file>` | Set primary output filename |
| `-e <symbol>` | Set entry symbol |
| `-r <start>-<end>`, `--reserve=<start>-<end>` | Reserve an address range |
| `-b <area>=<addr>`, `--section-start=<area>=<addr>` | Force a base address for a section/area |
| `-f <xl\|bin\|ihx>`, `--oformat=...` | Choose the primary output format |
| `-x <start>-<end>`, `--binary-range=<start>-<end>` | Restrict emitted BIN range |
| `-m`, `-M`, `--print-map` | Print the memory map |
| `-Map <file>`, `-Map=<file>` | Write the memory map to a file |
| `-v` | Verbose output |
| `--version` | Print the version |
| `-h`, `--help` | Show usage |

Input files are processed in command-line order. That matters for:

- normal `CON` area packing
- the order in which `.rel` modules are loaded
- the order in which overlaid bytes are copied into the final image

For `crt0`-style startup code, place the startup object first.

---

## Switch Reference

### `-g`

Write mode-specific debug output.

In `--mode=sdcc`, xld derives a linked `.cdb` sidecar with:

- compiler records copied from sibling module `.cdb` files when available
- fallback `M:`, `F:`, and `S:` compiler records synthesized from sibling `.adb` files when `.cdb` is missing
- linker `L:` records generated from the final linked addresses
- assembly `L:A...` line records from sibling `.lst` files

In `--mode=gnu`, xld derives an `.elf` sidecar with:

- a `.text` section derived from the final linked image
- a final symbol table
- `.debug_abbrev`, `.debug_info`, and `.debug_line` DWARF2 sections
- `.adb` for SDCC C debug information
- `.lst` for assembly source line mappings

If the sidecars are missing, xld still writes the selected mode-specific
debug sidecar, but it can only include what it knows from the linked
objects themselves.

Important detail for library modules:

- if a linked library function has symbol information but no real source
  file can be resolved on disk, xld still emits the function and symbol
  metadata
- in that case it intentionally omits bogus source file and line records
  instead of inventing a fake local file entry
- that lets debuggers fall back to symbol-level stepping and
  disassembly instead of trying to open a non-existent source file

### `--sdcc-runtime <dir>`

If present, xld modifies the input list before linking:

- it prepends the runtime `crt0`
- it appends the runtime default library

It prefers:

- `crt0.rel`
- `z80.lib`

If those exact names do not exist, it falls back to the only matching
`crt0*.rel` or `.lib` found in the directory.

If the switch is omitted, xld does nothing runtime-related.

### `-o <file>`

Sets the primary output path.

- with `-f xl`, this is the `XL` file
- with `-f bin`, this is the flat binary

It does not control the names of:

- the NoICE file from `-n`
- the CDB file from `-c`
- the debug file from `-g`

Those must be given explicitly.

### `-n <file>`

Writes a NoICE command file.

The file always includes linked `DEF` commands such as:

```text
DEF _symbol 0x1234
```

If sidecars are available, it also includes:

- `FILE` / `ENDFILE`
- `LINE`
- `FUNCTION` / `STATICFUNCTION` / `ENDFUNCTION`
- `DEFSCOPE` for file-local and local symbols when xld can recover them

This is useful for:

- NoICE source-level debugging
- external scripts that only care about the `DEF` lines
- quick inspection of final linked symbols

### `-e <symbol>`

Sets the entry symbol.

Default:

```text
_main
```

Common ROM/startup case:

```text
_entry
```

The entry symbol must exist after linking or xld stops with an error.

### `-r <start>-<end>`

Reserves an inclusive address range and prevents normal area placement
from using it.

This affects placement, not just file emission.

Example:

```bash
-r 4000-47FF
```

means that no normally placed area may occupy any byte between `0x4000`
and `0x47FF`.

Single-byte ranges are valid too:

```bash
-r 1708-1708
```

That reserves exactly one address.

#### How hole skipping works

When xld tries to place an area, it starts with a cursor and checks
whether the candidate interval overlaps any reserved hole. If it does,
the cursor jumps to `hole.end + 1` and the check repeats.

This is the actual algorithm in simplified form:

```cpp
uint16_t next_free_address(uint16_t cursor,
                           uint16_t size,
                           const std::vector<address_range>& holes) {
    while (true) {
        bool moved = false;
        uint16_t end = cursor + size - 1;

        for (const auto& hole : holes) {
            if (cursor <= hole.end && end >= hole.start) {
                cursor = hole.end + 1;
                moved = true;
                break;
            }
        }

        if (!moved)
            return cursor;
    }
}
```

Concrete example:

- area size = `0x20`
- current cursor = `0x0000`
- reserved hole = `0x0010..0x001F`

First candidate:

```text
0x0000..0x001F
```

That overlaps the hole, so xld jumps the cursor to:

```text
0x0020
```

Second candidate:

```text
0x0020..0x003F
```

That no longer overlaps, so the area is placed at `0x0020`.

#### Extra BIN behavior for reserved holes

For `-f xl`, reserved ranges only affect placement.

For `-f bin`, if a reserved hole lies inside the emitted BIN range, xld keeps the
reserved bytes zero-filled and, when possible, writes a jump immediately
before the hole:

- `JR` when the hole is small enough for an 8-bit relative skip
- `JP` when the hole is larger, if there are three bytes available before
  the hole

Important: when xld does this, it also treats the pre-hole jump bytes as
reserved during area placement:

- 2 bytes for `JR`
- 3 bytes for `JP`

That means later linked code is placed after them and relocated normally,
instead of being overwritten at BIN emit time.

If those final placement changes make an existing short branch (`JR`, conditional
`JR`, or `DJNZ`) no longer fit, xld now promotes it back to a safe long form
before relocation. This applies to both forward and backward jumps, so code that
crosses a reserved hole stays correct without manual padding.

That jump targets:

```text
hole_end + 1
```

So for a protected hole `0x0100..0x010F`, xld writes:

- `JR 0x0110` at `0x00FE..0x00FF`
- `0x00` bytes from `0x0100` through `0x010F`

For a single-byte protected address such as `0x1708..0x1708`, xld writes:

- `JR 0x1709` at `0x1706..0x1707`
- `0x00` at `0x1708`

If the hole begins too close to the start of the emitted BIN range, xld
skips that pre-hole jump because there is no room for it.

If the hole is too large for `JR`, xld uses `JP` instead when it can.
If there is not enough room for either form, xld leaves the entire
reserved range zero-filled and emits no pre-hole jump.

### `-b <area>=<addr>`

Pins the base address for an area group.

Example:

```bash
-b _CODE=0100
-b _DATA=5B00
```

This is applied at group placement time. If the requested base would move
backwards over already placed content, xld reports an error.

### `-f <xl|bin>`

Chooses the primary output format.

- `xl` is the default
- `bin` is for fixed-address raw images

Use:

- `xl` for loader-relocated programs
- `bin` for ROMs, memory images, or fixed-address executables

### `-x <start>-<end>`

Restricts the emitted file window for `-f bin`.

Important: in the normal case, `-x` only changes which address interval is
written to the output file.

Exception: with `-f bin` plus `-r`, xld uses the emitted BIN range to
decide whether it must reserve pre-hole bytes for a synthesized `JR` or
`JP`. So in that specific case, changing `-x` can indirectly affect
placement around reserved holes.

Without `-x`:

- BIN output starts at `0x0000`
- BIN output ends at the highest linked byte

That means non-zero origins can produce leading zero fill unless you crop
the output range with `-x`.

### `-m`

Prints the final memory map after linking.

The map shows:

- area name
- linked address
- size
- flags such as `ABS`, `REL`, `OVR`, `CON`

### `-v`

Turns on verbose output.

Today this includes:

- version banner
- file loading messages
- library inclusion messages
- resolved symbol count
- code size
- relocation count
- entry point
- final output size

### `-h`, `--help`

Prints the built-in help summary and exits.

---

## How Linking Works

### Pipeline

```text
.rel / .lib
    -> load objects
    -> resolve libraries on demand
    -> resolve global symbols
    -> place areas
    -> relocate bytes
    -> find entry point
    -> emit output files
```

### 1. Load inputs

Each `.rel` file is parsed into a module object.

Each `.lib` file is scanned, but its members are not loaded immediately.

### 2. Resolve libraries

xld performs demand-driven library linking:

- collect unresolved symbol references from already loaded modules
- scan library members for matching definitions
- load only the members that satisfy current unresolved symbols
- repeat until no more symbols can be satisfied

### 3. Resolve symbols

All `Def` symbols from loaded modules are inserted into the global table.

Errors:

- duplicate definition
- unresolved reference

Pseudo-symbols beginning with `.__.` are ignored for cross-module linking.

### 4. Place areas

Areas are grouped by name across all modules.

Within a name-group, xld uses the first area it sees to decide how the
group behaves:

- `ABS`: every member is placed at its own declared `org` address
- `OVR`: every member gets the same linked address; the group consumes the size of the largest member
- `CON`: members are packed one after another

#### Important note about `OVR`

`OVR` does **not** mean that xld writes separate overlay files.

It means only this:

- the same-named areas share one linked address
- relocation copies all their bytes into one final code buffer
- later copied bytes overwrite earlier copied bytes where they overlap

So the current `OVR` behavior is shared placement inside one output image,
not a separate overlay packaging system.

### 5. Relocate bytes

Each text record is copied to its placed address, then each relocation
entry patches the referenced byte or word.

Relocation modes include:

| Mode bit | Meaning |
|----------|---------|
| bit 0 | word vs byte relocation |
| bit 1 | symbol reference vs area reference |
| bit 2 | PC-relative relocation |
| bit 7 | high-byte-only patch |

For non-PC-relative relocations, xld also records the patch in the `XL`
relocation table.

### 6. Find the entry point

The symbol named by `-e` is looked up in the final global symbol table.

Its area-relative value is combined with the linked area address to produce
the linked entry address stored in the output.

### 7. Emit output files

- primary output: `XL` or `BIN`
- optional NoICE file: `-n`
- optional CDB file: `-c`
- optional debug database: `-g`

---

## Input File Formats

### SDCC `.rel` object module

xld understands both the classic SDCC v1 format and the extended `XL4`
format produced by SDCC 4.x.

#### Record types

| Record | Meaning |
|--------|---------|
| `XL` | Little-endian byte order (v1). |
| `XL4` | Little-endian, extended 32-bit format (v4, SDCC 4.x). |
| `XH` | Big-endian byte order (parsed, but not a normal Z80 target case). |
| `H <n> areas <n> global symbols` | Informational header. Counts are not enforced. |
| `M <name>` | Module name. |
| `O <flags>` | Compiler or assembler flags. Ignored by xld. |
| `A <name> size <hex> flags <hex> [addr <hex>]` | Area declaration. |
| `S <name> Def<hex>` | Symbol definition. |
| `S <name> Ref<hex>` | Symbol reference. |
| `T ...` | Text bytes for an area. |
| `R ...` | Relocations for the preceding `T` record. |

#### Area flags

| Bit | Meaning |
|-----|---------|
| `0x01` | `OVR` overlay bit |
| `0x08` | `ABS` absolute bit in SDCC 4.x |
| `0x04` | legacy `ABS` bit |

Anything else is treated as normal relocatable concatenated placement.

#### T / R record layouts

**v1 (`XL`):**

```text
T <off_lo> <off_hi> <byte> ...
R <b0> <b1> <area_lo> <area_hi> [<mode> <off_lo> <off_hi> <ref>] ...
```

**v4 (`XL4`):**

```text
T <b0> <b1> <b2> <b3> <byte> ...
R <b0> <b1> <area_lo> <area_hi> [<mode> <from_T_start> <ref_lo> <ref_hi>] ...
```

For Z80, only the low 16 bits matter for addresses.

### `.lib` libraries

xld understands two library layouts:

1. xld text-index library
2. native `ar`-style SDCC library archive

#### Text-index `.lib`

One relative `.rel` path per line:

```text
# example
modules/printf.rel
modules/memcpy.rel
modules/divuint.rel
```

Blank lines and lines starting with `#` are ignored.

#### Native SDCC archive `.lib`

This is a normal `ar` archive that contains `.rel` members.

xld extracts the `.rel` members logically and considers them for
demand-driven resolution the same way as text-index libraries.

### `.adb` debug sidecars

Used by `-g`, and as a fallback compiler-record source for `-c` if no
matching sibling `.cdb` exists.

They provide SDCC debug metadata such as:

- function names
- return types
- globals
- locals
- register or frame-relative storage

### `.cdb` debug sidecars

Used by `-c`.

When present, xld copies the compiler-generated `M:`, `F:`, `S:`, and
`T:` records from these files and then appends new linker `L:` records for
the final linked addresses.

### `.lst` assembler listings

Used by `-g`, `-n`, and `-c`.

They provide line mapping for assembly modules, so `xgdb` can show source
locations for assembly code, NoICE can get `LINE` commands, and linked
`.cdb` output can get `L:A...` line records.

---

## Output File Formats

### NoICE `.noi` output (`-n`)

The `.noi` file is a NoICE command file.

It always includes linked `DEF` commands such as:

```text
LASTFILELOADED
CLEARLINEINFO Y
DEF _entry 0x0100
DEF _main 0x0134
DEF s__CODE 0x0100
DEF l__CODE 0x01A4
```

If source sidecars are available, xld also writes:

- `FILE` / `ENDFILE`
- `LINE`
- `FUNCTION` / `ENDFUNCTION`
- `DEFSCOPE`

The `DEF` lines are kept in the file specifically so simple in-repo tools
can still load final symbol addresses from it without understanding the
rest of the NoICE command language.

### Linked SDCC `.cdb` output (`-c`)

This is a linked SDCC CDB file.

At a high level it contains:

- compiler `M:`, `F:`, `S:`, and `T:` records from sibling module `.cdb` files
- fallback `M:`, `F:`, and `S:` records normalized from sibling `.adb` files when a module `.cdb` is missing
- linker `L:` address records for linked symbols and functions
- linker `L:C...` source line records from C line symbols in `.rel`
- linker `L:A...` assembly line records from sibling `.lst` files

### `.xgdb` debug sidecar (`-g`)

This is the linked debug database consumed by `xgdb`.

At a high level it contains:

- the linked image path
- the linked entry address
- source files
- symbols
- function ranges
- line mappings
- local variable metadata

Typical sources for that data:

- `.rel` for final linked addresses
- `.adb` for SDCC C debug details
- `.lst` for assembly line mappings

### `XL` relocatable output (`-f xl`, default)

All multi-byte fields are little-endian.

#### Header — 12 bytes

| Offset | Size | Field | Meaning |
|--------|------|-------|---------|
| 0 | 2 | Magic | `'X' 'L'` |
| 2 | 1 | Version | currently `0x01` |
| 3 | 1 | Flags | reserved, currently `0x00` |
| 4 | 2 | `entry_point` | linked entry address inside the emitted image |
| 6 | 2 | `code_size` | total emitted code/data span |
| 8 | 2 | `reloc_count` | number of relocation entries |
| 10 | 2 | Reserved | `0x0000` |

#### Relocation table

Each relocation entry is 4 bytes:

| Offset | Size | Field | Meaning |
|--------|------|-------|---------|
| 0 | 2 | `offset` | byte offset into payload |
| 2 | 1 | `size` | `1` = byte, `2` = word |
| 3 | 1 | `pad` | bit0: for byte relocations, patch MSB instead of LSB |

Only non-PC-relative absolute relocations appear here.

For byte relocations:
- `size = 1`, `pad bit0 = 0` means add the load base low byte
- `size = 1`, `pad bit0 = 1` means add the load base high byte

Word relocations keep `size = 2` and currently leave `pad = 0`.

#### Code/data payload

The payload is the linked image bytes.

If your linked code starts at a non-zero address, bytes below that address
still exist in the emitted payload as zero-filled space, because the image
represents the linked address space from `0x0000` up to `code_size - 1`.

#### Load-time idea

```text
read XL header
read relocation table
load payload at chosen base
patch each relocation by adding that base
jump to base + entry_point
```

### `BIN` flat absolute output (`-f bin`)

This is a raw byte image with no `XL` header and no relocation table.

The bytes are already linked for the addresses you chose at link time.

Important:

- xld does not relocate a BIN at load time
- your loader or ROM builder must place the BIN exactly where you linked it for

If `-x` is given, xld emits exactly that inclusive address interval.

If the emitted BIN range contains reserved holes from `-r`, the reserved
bytes stay zero-filled. When possible, xld also synthesizes a `JR` or `JP`
immediately before each hole so execution can skip over it without touching
the reserved bytes themselves.

If `-x` is omitted:

- BIN starts at `0x0000`
- BIN ends at the highest linked byte

That means non-zero origins can create leading zero-fill unless you crop
the file with `-x`.

---

## Build and Test

From `src/xc/xld`:

```bash
make
make test
make clean
```

This builds:

```text
bin/x/bin/xld
```

The test suite covers:

- `.rel` parsing
- `.lib` parsing
- area placement
- reserved holes
- relocation
- symbol resolution
- BIN emission
- runtime injection
- `.noi` emission
- `.cdb` emission
- `.xgdb` emission

---

## Practical Summary

If you just want the shortest mental model:

- `.rel` and `.lib` go in
- xld resolves symbols and places areas
- `-f xl` gives you a relocatable loader image
- `-f bin` gives you a fixed-address raw image
- `-n` gives you a NoICE command file
- `-c` gives you a linked SDCC `.cdb` file
- `-g` gives you a linked `.xgdb` debugger sidecar
- `-r` blocks placement in reserved holes and, for `BIN`, can synthesize a pre-hole `JR` or `JP`
- `-x` crops the BIN file window
- `-b` pins area groups to known addresses
- `--sdcc-runtime` auto-injects a runtime `crt0` and default library only when you ask for it

That is the core of xld.
