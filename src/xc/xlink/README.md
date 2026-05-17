# xlink — Z80 Relocatable Linker

`xlink` is the XYZ toolchain linker for Z80 targets. It reads SDCC-style
textual `.rel` object modules and `.lib` library archives, resolves symbols,
places areas in memory, applies relocations, and emits either:

- relocatable **XL format** output (default), or
- flat absolute **BIN** output for ROM images.

---

## How It Works

### Pipeline

```
.rel / .lib  →  load  →  library resolve  →  symbol resolve
                                                     ↓
              emit XL  ←  relocate  ←  place areas
```

#### 1. Load inputs

Each `.rel` file on the command line is parsed immediately into a `module`
object. `.lib` files are scanned but not yet loaded — their member modules
are catalogued for the next stage.

#### 2. Library resolution

xlink performs demand-driven linking: it collects all undefined symbol
references from already-loaded modules and then scans library modules for
definitions that satisfy them. Any satisfying module is loaded, which may
introduce new undefined symbols, so the process repeats until a fixpoint is
reached. Only modules actually needed end up in the link.

#### 3. Symbol resolution

All `Def` symbols from every loaded module are collected into a global
symbol table. Errors are reported for:

- **duplicate definitions** — the same name defined in two different modules.
- **unresolved references** — a `Ref` symbol that has no matching `Def`.

SDCC internal pseudo-symbols (names beginning with `.__.`) are silently
ignored; they exist in every module but carry no inter-module meaning.

#### 4. Area placement

Areas with the same name across modules are grouped. Within each group:

- **ABS** areas are placed at their declared `org` address.
- **OVR** (overlay) areas all start at the same address; the group occupies
  the size of the largest member.
- **CON** (concatenate, the default) areas are packed sequentially, one
  after another, in the order their modules appear on the command line.

Placement starts at address `0` and advances a cursor. Reserved ranges
supplied via `-r` are skipped: the cursor jumps past any hole that would
overlap the area being placed.

The total span of all placed areas becomes `code_size` in the output header.

#### 5. Relocation

Each T record's bytes are copied to the code buffer at the area's placed
address. Relocation entries in the accompanying R records are then applied:

| Mode bits | Meaning |
|-----------|---------|
| `word` (bit 0) | patch 2 bytes (little-endian 16-bit) |
| `sym` (bit 1) | reference is a symbol index; otherwise an area index |
| `pc_rel` (bit 2) | subtract the address of the byte after the field (relative branch) |
| `msb` (bit 7) | patch only the high byte of the 16-bit value |

For each non-`pc_rel` word relocation, an entry is added to the output
relocation table so the OS loader knows which bytes to fix up when the
binary is loaded at a non-zero address.

#### 6. Entry point

The symbol named by `-e` (default `_main`) is looked up in the global
symbol table. Its area-relative value is added to the placed address of
the defining module's first area to yield the absolute entry point written
into the XL header.

#### 7. Emit

The XL binary is written: header, relocation table, code buffer.

---

## Command-Line Usage

```
xlink [options] <file.rel|file.lib> ...
```

| Option | Description |
|--------|-------------|
| `-o <file>` | Output file path. Default: `a.out`. |
| `-n <file>` | Write symbols as `DEF name 0xADDR` lines. |
| `-e <symbol>` | Entry point symbol name. Default: `_main`. |
| `-r <start>-<end>` | Reserve an address range (hex, inclusive). Placement skips this range. Repeatable. |
| `-b <area>=<addr>` | Force base address for an area group (hex). Repeatable. |
| `-f <xl|bin>` | Output format. Default: `xl`. |
| `-x <start>-<end>` | Output range for `-f bin` (hex, inclusive). |
| `-m` | Print a memory map after linking (area names, placed addresses, sizes, flags). |
| `-v` | Verbose output: log each file loaded, symbol counts, code size, entry point. |
| `-h`, `--help` | Show usage summary and exit. |

Input files are processed in command-line order. The order determines
CON area packing: the first module's `_CODE` comes first, and so on.
For `crt0`-style startup code, always list the CRT0 module first.

### Example

```bash
xlink -v -m -e _entry \
      -r 0000-003F \
      -b _CODE=0100 \
      -o prog.xl \
      build/crt0.rel build/main.rel build/runtime.lib
```

This reserves the bottom 64 bytes (e.g. a Z80 interrupt vector table),
uses `_entry` as the jump target for the OS loader, and prints a full
memory map.

---

## Input File Formats

### SDCC `.rel` object module

xlink understands both the classic SDCC v1 format and the extended v4
format (`XL4`) produced by SDCC 4.x.

#### Record types

| Record | Meaning |
|--------|---------|
| `XL` | Little-endian byte order (v1). |
| `XL4` | Little-endian, extended 32-bit format (v4, SDCC 4.x). |
| `XH` | Big-endian byte order (unsupported target, parsed only). |
| `H <n> areas <n> global symbols` | Informational header line; counts are not enforced. |
| `M <name>` | Module name. |
| `O <flags>` | Compiler/assembler flags (ignored). |
| `A <name> size <hex> flags <hex> [addr <hex>]` | Area declaration. |
| `S <name> Def<hex>` | Symbol definition (area-relative value). |
| `S <name> Ref<hex>` | Symbol reference (value unused). |
| `T ...` | Text record: code/data bytes. |
| `R ...` | Relocation record for the preceding T record. |

#### Area flags

| Bit | Meaning |
|-----|---------|
| 0 (`0x01`) | `OVR` — overlay mode; all same-named areas share one address. |
| 3 (`0x08`) | `ABS` — absolute; placed at the address given in `addr` (SDCC 4.x). |
| 2 (`0x04`) | `ABS` legacy bit, still accepted for compatibility. |

All other combinations are treated as `CON REL` (concatenate, relocatable).

#### T / R record formats

**v1 (XL):**

```
T <off_lo> <off_hi> <byte> ...
R <b0> <b1> <area_lo> <area_hi> [<mode> <off_lo> <off_hi> <ref>] ...
```

- T offset: 2-byte little-endian, relative to the start of the area.
- R header: 4 bytes; area index in bytes 2–3 (little-endian).
- R reloc entry: 4 bytes `[mode][off_lo][off_hi][ref]`.
  - `off` is 2-byte little-endian, relative to the start of T data.
  - `ref` is a 1-byte area or symbol index.
  - mode bit 0 = 1 → word (16-bit) relocation.

**v4 (XL4, SDCC 4.x):**

```
T <b0> <b1> <b2> <b3> <byte> ...
R <b0> <b1> <area_lo> <area_hi> [<mode> <from_T_start> <ref_lo> <ref_hi>] ...
```

- T offset: 4-byte little-endian (high 2 bytes are always zero for Z80).
- R header: unchanged — area index is still bytes 2–3.
- R reloc entry: 4 bytes `[mode][from_T_start][ref_lo][ref_hi]`.
  - `from_T_start` is the byte offset measured from the **start of the T
    record** (including the 4-byte offset field); subtract 4 to get the
    offset into the data payload.
  - `ref` is 2-byte little-endian area or symbol index.
  - mode bit 0 = **0** → word (16-bit) relocation (inverted vs. v1).

Symbol values in S records are 8 hex digits in v4 (truncated to 16 bits
for Z80).

### SDCC `.lib` library archive

A plain text file with one relative path per line, each pointing to a
`.rel` member module. Paths are resolved relative to the directory
containing the `.lib` file.

```
# example library index
modules/printf.rel
modules/memcpy.rel
modules/divuint.rel
```

Lines starting with `#` and blank lines are ignored.

---

## Output File Formats

### XL (default)

All multi-byte fields are little-endian.

All multi-byte fields are little-endian.

### Header — 12 bytes

| Offset | Size | Field | Value |
|--------|------|-------|-------|
| 0 | 2 | Magic | `0x58 0x4C` (`'X' 'L'`) |
| 2 | 1 | Version | `0x01` |
| 3 | 1 | Flags | `0x00` (reserved) |
| 4 | 2 | `entry_point` | Offset of the entry symbol within the code payload |
| 6 | 2 | `code_size` | Total size of the code/data payload in bytes |
| 8 | 2 | `reloc_count` | Number of relocation entries that follow the header |
| 10 | 2 | Reserved | `0x0000` |

### Relocation table — `reloc_count × 4` bytes

Each entry describes one word or byte in the payload that must be adjusted
when the binary is loaded at a non-zero base address.

| Offset | Size | Field | Meaning |
|--------|------|-------|---------|
| 0 | 2 | `offset` | Byte offset into the code payload |
| 2 | 1 | `size` | `1` = byte patch, `2` = word patch (16-bit little-endian) |
| 3 | 1 | `pad` | `0x00` (reserved) |

Only non-PC-relative absolute relocations appear in this table.
PC-relative relocations (e.g. `JR`, `DJNZ`) are fully resolved at link
time and need no run-time adjustment.

### Code/data payload — `code_size` bytes

### BIN (`-f bin`)

Flat absolute binary without an XL header. Bytes are emitted directly from
the linked address space. If `-x` is provided, xlink emits exactly that
address interval and fills missing bytes with `0x00`.

The raw, pre-relocated binary image. All internal cross-references have
already been resolved; the only thing remaining for the loader is to add
the load address to each field listed in the relocation table.

### Loading algorithm

```
base = load_address_chosen_by_OS

for each entry in reloc_table:
    if entry.size == 2:
        patch_u16(payload + entry.offset, read_u16(payload + entry.offset) + base)
    else:
        patch_u8 (payload + entry.offset, read_u8 (payload + entry.offset) + base)

jump to (base + header.entry_point)
```

### Example: `hello.xl`

Produced by linking `crt0.rel` + `hello.rel`, loaded at address 0 (base 0):

```
Header:
  magic      = 'XL'
  version    = 0x01
  entry      = 0x0000
  code_size  = 0x008D  (141 bytes)
  reloc_cnt  = 2

Reloc table:
  [offset=0x0001, size=2]   ← LD SP, nn  (SP target)
  [offset=0x0004, size=2]   ← CALL nn    (main address)

Code payload (first 13 bytes = _CODE):
  31 8D 00   LD SP, 0x008D   ; __stack_top
  CD 09 00   CALL 0x0009     ; _main
  76         HALT
  18 FD      JR -3           ; loop
  11 00 00   LD DE, #0       ; hello's return 0
  C9         RET
```

If loaded at address `0x8000`:
- `LD SP` gets `0x8000 + 0x008D = 0x808D`
- `CALL` gets `0x8000 + 0x0009 = 0x8009`

---

## Build and Test

```bash
# From the xlink directory (or via the repo root make):
make          # builds bin/bin/xc/xlink/xlink
make test     # runs 20 unit + integration tests
make clean    # removes build artifacts
```

The test suite covers: `.rel` parsing (v1 format), area placement (CON/OVR/ABS,
holes), relocation, symbol resolution, library selective inclusion, and
full binary output.

---

## Compatibility Notes

- **SDCC v4 (XL4)** support was added for the T/R extended record format used
  by SDCC 4.x toolchains. v1 (`XL`) format remains fully supported.
- Cross-module symbol placement assumes the defining symbol's value is
  relative to the defining module's **first area** (`areas()[0]`). This
  matches standard `_CODE`-first layouts; unusual area ordering could
  produce incorrect entry-point calculations.
- `.lib` parsing treats each non-blank, non-comment line as a `.rel` path.
  The format used in this project's `z80.lib` (SDCC's runtime library) is
  compatible with this scheme.
