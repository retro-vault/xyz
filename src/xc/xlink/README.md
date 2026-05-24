# xlink — Z80 Relocatable Linker

`xlink` is the XYZ toolchain linker for Z80 targets.

In plain words, it does four main jobs:

1. read SDCC-style `.rel` object files and `.lib` libraries
2. resolve symbols and pull in only the library modules that are needed
3. place areas in memory while respecting base addresses and reserved holes
4. write one or more output files for loading, ROM building, symbols, and debugging

What `xlink` can do today:

- link `.rel` files produced from both C and assembly
- read two `.lib` styles:
  - xlink text-index libraries
  - native `ar`-style SDCC archives that contain `.rel` members
- place relocatable, absolute, and overlay areas
- skip reserved address ranges during placement
- emit relocatable `XL` output
- emit flat absolute `BIN` output
- emit a NoICE `.noi` command file
- emit a linked SDCC `.cdb` debug file
- emit a linked `.xdbg` debug sidecar
- optionally prepend a runtime `crt0` and append a runtime library from `--sdcc-runtime <dir>`

Current limits that are worth knowing up front:

- there is no real banked-memory output format yet
- `OVR` is only shared-address placement inside one final image
- `OVR` does not produce separate overlay payload files
- if two overlaid modules write bytes to the same addresses, later copied bytes overwrite earlier ones in the final image

---

## What Files xlink Reads

| File | Purpose | Required |
|------|---------|----------|
| `.rel` | Main object format. Contains areas, symbols, code bytes, and relocations. | Yes |
| `.lib` | Library of `.rel` modules. Can be either xlink text-index format or a native SDCC `ar` archive. xlink loads only members that satisfy unresolved symbols. | Optional |
| `.adb` | SDCC C debug sidecar. Used by `.xdbg`, and as a fallback source for compiler records when emitting `.cdb`. | Optional |
| `.cdb` | SDCC compiler debug sidecar. Preferred compiler-record source when emitting a linked `.cdb`. | Optional |
| `.lst` | Assembler listing. Used when emitting `.xdbg`, `.noi`, or `.cdb` for assembly modules. | Optional |

### Notes

- `.rel` is the real linker input.
- Both C and assembly reach xlink as `.rel`; the difference matters only when optional debug sidecars are collected.
- `.adb`, `.cdb`, and `.lst` are not linked themselves; they are sidecars used only to enrich debug outputs.
- `.adb` is usually present for SDCC C compilation with debug enabled.
- `.cdb` is usually present for SDCC C compilation with `--debug`.
- `.lst` is usually present for assembler output with listing/debug enabled.

---

## What Files xlink Writes

| File | How You Ask For It | What It Is |
|------|---------------------|------------|
| primary output, default `a.out` | always, via `-o <file>` | Either relocatable `XL` or flat `BIN`, depending on `-f` |
| `.xl` | default `-f xl` | Relocatable XYZ loader image with header and relocation table |
| `.bin` | `-f bin` | Flat absolute binary image |
| `.noi` | `-n <file>` | NoICE command file with `DEF`, `FILE`, `LINE`, and scope records |
| `.cdb` | `-c <file>` or `--cdb <file>` | Linked SDCC CDB debug file |
| `.xdbg` | `-g <file>` or `--xdbg <file>` | Linked debug database for `xdbg` |

### Which one should I use?

- Use `XL` when a loader will relocate the program at load time.
- Use `BIN` when you want a fixed-address ROM or raw memory image.
- Use `-n` when you want NoICE-compatible symbols and source lines.
- Use `-c` when you want a linked SDCC-native debug file.
- Use `-g` when you want source-level debugging metadata.

---

## Quick Start

### 1. Produce a relocatable XL file

```bash
xlink -o hello.xl \
      build/crt0.rel build/hello.rel
```

This writes relocatable `XL` output and uses `_main` as the entry symbol.

### 2. Produce a pure flat binary

```bash
xlink -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

This links `_CODE` at `0x0100` and emits only the inclusive range
`0x0100..0x02FF` into `hello.bin`.

### 3. Produce a binary plus a NoICE file

```bash
xlink -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -n build/hello.noi \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

The `.noi` file now contains NoICE commands. The exact contents depend on
which sidecars exist, but the linked symbol section still looks familiar:

```text
LASTFILELOADED
CLEARLINEINFO Y
DEF _entry 0x0100
DEF _main 0x0134
DEF s__CODE 0x0100
DEF l__CODE 0x01A4
```

If C and assembly sidecars are present, xlink also adds `FILE`, `LINE`,
`FUNCTION`, and `DEFSCOPE` commands.

### 4. Produce a binary plus a linked SDCC CDB file

```bash
xlink -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -c build/hello.cdb \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

For the `.cdb` file to be rich:

- C modules should have sibling `.cdb` files from SDCC `--debug`
- if a sibling `.cdb` is missing, xlink falls back to sibling `.adb`
- assembly modules should have sibling `.lst` files if you want `L:A...` line records

### 5. Produce a binary plus an xdbg file

```bash
xlink -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -g build/hello.xdbg \
      -o hello.bin \
      build/crt0.rel build/hello.rel
```

For the `.xdbg` file to be rich:

- C modules should have sibling `.adb` files
- assembly modules should have sibling `.lst` files

### 6. Use an SDCC runtime directory only when you want it

```bash
xlink --sdcc-runtime /path/to/runtime/z80 \
      -f bin -e _entry \
      -b _CODE=0100 \
      -x 0100-02FF \
      -o hello.bin \
      build/hello.rel
```

If `--sdcc-runtime` is not given, xlink does not inject any startup object
or library on its own.

### 7. Reserve holes and print a memory map

```bash
xlink -v -m -f bin -x 0000-3FFF -e _entry \
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
xlink [options] <file.rel|file.lib> ...
```

There are thirteen switches in total today.

| Option | Short meaning |
|--------|---------------|
| `-c`, `--cdb <file>` | Write linked SDCC `.cdb` debug output |
| `-g`, `--xdbg <file>` | Write linked `.xdbg` debug output |
| `--sdcc-runtime <dir>` | Auto-inject runtime `crt0` and default library from a directory |
| `-o <file>` | Set primary output filename |
| `-n <file>` | Write NoICE `.noi` output |
| `-e <symbol>` | Set entry symbol |
| `-r <start>-<end>` | Reserve an address range |
| `-b <area>=<addr>` | Force base address for an area group |
| `-f <xl\|bin>` | Choose output format |
| `-x <start>-<end>` | Restrict emitted BIN range |
| `-m` | Print memory map |
| `-v` | Verbose output |
| `-h`, `--help` | Show usage |

Input files are processed in command-line order. That matters for:

- normal `CON` area packing
- the order in which `.rel` modules are loaded
- the order in which overlaid bytes are copied into the final image

For `crt0`-style startup code, place the startup object first.

---

## Switch Reference

### `-c`, `--cdb <file>`

Write a linked SDCC `.cdb` sidecar.

What xlink writes into it:

- compiler records copied from sibling module `.cdb` files when available
- fallback `M:`, `F:`, and `S:` compiler records synthesized from sibling `.adb` files when `.cdb` is missing
- linker `L:` records generated from the final linked addresses
- assembly `L:A...` line records from sibling `.lst` files

This is the closest output to SDCC's native linked debug format.

### `-g`, `--xdbg <file>`

Write a linked `.xdbg` sidecar.

What it contains:

- image path
- entry address
- source file table
- linked symbols
- function ranges
- line mappings
- local variable metadata when available

Where the data comes from:

- `.rel` symbols and linked addresses
- `.adb` for SDCC C debug information
- `.lst` for assembly source line mappings

If the sidecars are missing, xlink still writes `.xdbg`, but it can only
include what it knows from the linked objects themselves.

Important detail for library modules:

- if a linked library function has symbol information but no real source
  file can be resolved on disk, xlink still emits the function and symbol
  metadata
- in that case it intentionally omits bogus source file and line records
  instead of inventing a fake local file entry
- that lets debuggers fall back to symbol-level stepping and
  disassembly instead of trying to open a non-existent source file

### `--sdcc-runtime <dir>`

If present, xlink modifies the input list before linking:

- it prepends the runtime `crt0`
- it appends the runtime default library

It prefers:

- `crt0.rel`
- `z80.lib`

If those exact names do not exist, it falls back to the only matching
`crt0*.rel` or `.lib` found in the directory.

If the switch is omitted, xlink does nothing runtime-related.

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
- `DEFSCOPE` for file-local and local symbols when xlink can recover them

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

The entry symbol must exist after linking or xlink stops with an error.

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

When xlink tries to place an area, it starts with a cursor and checks
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

That overlaps the hole, so xlink jumps the cursor to:

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

For `-f bin`, if a reserved hole lies inside the emitted BIN range, xlink keeps the
reserved bytes zero-filled and, when possible, writes a jump immediately
before the hole:

- `JR` when the hole is small enough for an 8-bit relative skip
- `JP` when the hole is larger, if there are three bytes available before
  the hole

Important: when xlink does this, it also treats the pre-hole jump bytes as
reserved during area placement:

- 2 bytes for `JR`
- 3 bytes for `JP`

That means later linked code is placed after them and relocated normally,
instead of being overwritten at BIN emit time.

That jump targets:

```text
hole_end + 1
```

So for a protected hole `0x0100..0x010F`, xlink writes:

- `JR 0x0110` at `0x00FE..0x00FF`
- `0x00` bytes from `0x0100` through `0x010F`

For a single-byte protected address such as `0x1708..0x1708`, xlink writes:

- `JR 0x1709` at `0x1706..0x1707`
- `0x00` at `0x1708`

If the hole begins too close to the start of the emitted BIN range, xlink
skips that pre-hole jump because there is no room for it.

If the hole is too large for `JR`, xlink uses `JP` instead when it can.
If there is not enough room for either form, xlink leaves the entire
reserved range zero-filled and emits no pre-hole jump.

### `-b <area>=<addr>`

Pins the base address for an area group.

Example:

```bash
-b _CODE=0100
-b _DATA=5B00
```

This is applied at group placement time. If the requested base would move
backwards over already placed content, xlink reports an error.

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

Exception: with `-f bin` plus `-r`, xlink uses the emitted BIN range to
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

xlink performs demand-driven library linking:

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

Within a name-group, xlink uses the first area it sees to decide how the
group behaves:

- `ABS`: every member is placed at its own declared `org` address
- `OVR`: every member gets the same linked address; the group consumes the size of the largest member
- `CON`: members are packed one after another

#### Important note about `OVR`

`OVR` does **not** mean that xlink writes separate overlay files.

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

For non-PC-relative relocations, xlink also records the patch in the `XL`
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

xlink understands both the classic SDCC v1 format and the extended `XL4`
format produced by SDCC 4.x.

#### Record types

| Record | Meaning |
|--------|---------|
| `XL` | Little-endian byte order (v1). |
| `XL4` | Little-endian, extended 32-bit format (v4, SDCC 4.x). |
| `XH` | Big-endian byte order (parsed, but not a normal Z80 target case). |
| `H <n> areas <n> global symbols` | Informational header. Counts are not enforced. |
| `M <name>` | Module name. |
| `O <flags>` | Compiler or assembler flags. Ignored by xlink. |
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

xlink understands two library layouts:

1. xlink text-index library
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

xlink extracts the `.rel` members logically and considers them for
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

When present, xlink copies the compiler-generated `M:`, `F:`, `S:`, and
`T:` records from these files and then appends new linker `L:` records for
the final linked addresses.

### `.lst` assembler listings

Used by `-g`, `-n`, and `-c`.

They provide line mapping for assembly modules, so `xdbg` can show source
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

If source sidecars are available, xlink also writes:

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

### `.xdbg` debug sidecar (`-g`)

This is the linked debug database consumed by `xdbg`.

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
| 3 | 1 | `pad` | reserved |

Only non-PC-relative absolute relocations appear here.

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

- xlink does not relocate a BIN at load time
- your loader or ROM builder must place the BIN exactly where you linked it for

If `-x` is given, xlink emits exactly that inclusive address interval.

If the emitted BIN range contains reserved holes from `-r`, the reserved
bytes stay zero-filled. When possible, xlink also synthesizes a `JR` or `JP`
immediately before each hole so execution can skip over it without touching
the reserved bytes themselves.

If `-x` is omitted:

- BIN starts at `0x0000`
- BIN ends at the highest linked byte

That means non-zero origins can create leading zero-fill unless you crop
the file with `-x`.

---

## Build and Test

From `src/xc/xlink`:

```bash
make
make test
make clean
```

This builds:

```text
bin/bin/xlink
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
- `.xdbg` emission

---

## Practical Summary

If you just want the shortest mental model:

- `.rel` and `.lib` go in
- xlink resolves symbols and places areas
- `-f xl` gives you a relocatable loader image
- `-f bin` gives you a fixed-address raw image
- `-n` gives you a NoICE command file
- `-c` gives you a linked SDCC `.cdb` file
- `-g` gives you a linked `.xdbg` debugger sidecar
- `-r` blocks placement in reserved holes and, for `BIN`, can synthesize a pre-hole `JR` or `JP`
- `-x` crops the BIN file window
- `-b` pins area groups to known addresses
- `--sdcc-runtime` auto-injects a runtime `crt0` and default library only when you ask for it

That is the core of xlink.
