# xprog — XPRG and retro media packager

`xprog` validates an XL image and prefixes it with the XPRG version 1
descriptor. It creates either a `.prc` process image or a `.svc` resident
service image. It does not link programs, resolve symbols, or install either
kind of image.

It also wraps a flat ZX Spectrum binary as an auto-running TAP or TZX image.
Those modes emit a tokenized BASIC loader followed by a CODE block; they do
not require an external tape-image library. For a complete target build and
Fuse workflow, see the [ZX Spectrum 48K guide](../../docs/howtos/ZX-SPECTRUM-48K.md).

For Amstrad CPC programs it creates firmware-loadable CDT cassette images or
standard CPCEMU/AMSDOS DSK data disks. See the
[Amstrad CPC guide](../../docs/howtos/AMSTRAD-CPC.md).

For YOS and other divIDE/esxDOS workflows it can create a raw 16 MiB IDE disk
with an MBR, one bootable FAT16 partition, and one caller-named 8.3 file.

## Usage

```sh
xprog --process app.xl --stack-size 1024
xprog --service runtime.xl --load-address 0xfd00 --fixed-load \
    --export 0x0010 --export 0x0038
xprog --inspect app.prc
xprog --tap hello.bin --load-address 0x5ccb
xprog --tzx hello.bin --load-address 0x5ccb --entry 0x5ccb
xprog --cdt hello.bin --name HELLO
xprog --dsk hello.bin --name HELLO.BIN
xprog --esxdos shell.sys --name SHELL.SYS -o yos.ide
```

The default output name replaces `.xl` with `.prc` or `.svc`. Use `-o` to
choose another path. Numbers accept decimal or a `0x` hexadecimal prefix.
Running `xprog` without arguments prints the complete usage text.

For `--tap` and `--tzx`, the default load address and entry point are both
`0x5CCB`, the first address after the 48K ROM system variables. Tape names are
limited to the Spectrum header's ten bytes. An auto-start BASIC line calls a
30-byte loader stored after `REM`; it reads the following CODE header and data
with the 48K ROM routine, leaves the final entry address on the stack, and
lets the ROM return directly into the program. The CODE block can therefore
overwrite the BASIC program and still start at `0x5CCB`. Every TAP block
carries the standard XOR checksum. TZX output uses version 1.20
standard-speed data blocks, so it is accepted by ROM loaders and timing-aware
emulators.

Tape mode rejects empty input, binaries that cross `0xFFFF`, entry points
outside the binary, zero load addresses, and names outside the Spectrum
header's 1–10 byte limit. Process/service-only metadata switches are also
rejected in tape mode.

For `--cdt` and `--dsk`, the load and entry addresses default to `0x4000`.
CDT output uses CPC firmware binary headers and 2 KiB records inside a
TZX-compatible CDT 1.20 container; names allow 16 printable bytes. DSK output
uses the standard 40-track, single-sided CPCEMU format with the conventional
AMSDOS data-disk sector layout. It creates one 8.3 binary file with a valid
AMSDOS header, logical length, load/entry addresses, and checksum.

CPC media modes reject empty input, binaries that cross `0xFFFF`, entries
outside the loaded range, malformed names, and process/service-only metadata.
The resulting CDT and DSK images are directly boot-tested by the 464, 664,
and 6128 MCP regression.

`--esxdos` accepts any nonempty input file and writes it unchanged as the only
root-directory entry. The default output extension is `.ide`; the default
disk filename is the full input filename. Names are uppercased in the FAT
directory and must be valid 8.3 names. The image is deterministic: a 16 MiB
raw disk, 1 MiB-aligned 15 MiB FAT16 partition, 512-byte clusters, two FATs,
and 512 root entries. Process/service and load-address options are rejected in
this mode. Attach the resulting file as an IDE disk to divIDE/esxDOS hardware
or an emulator.

The process entry defaults to the entry offset recorded by XL. A service has
no initializer unless `--entry` is supplied. Each `--export` appends one JP
entry in ABI slot order; it names an offset in the XL code payload, not a
link-time absolute address.

## XPRG version 1

All integers are little-endian. The fixed descriptor is 64 bytes:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | `XPRG` magic |
| 4 | 1 | format version (`1`) |
| 5 | 1 | kind: process (`1`) or service (`2`) |
| 6 | 1 | image ABI version |
| 7 | 1 | flags: fixed load, entry present, JP table present |
| 8 | 2 | metadata size |
| 10 | 2 | XL payload file offset (equal to metadata size in v1) |
| 12 | 4 | XL payload size |
| 16 | 4 | IEEE CRC-32 of the complete XL payload |
| 20 | 4 | stable image ID |
| 24 | 2 | preferred load address |
| 26 | 2 | XL code-relative entry, or `0xffff` for none |
| 28 | 2 | process stack size; zero for a service |
| 30 | 2 | minimum OS ABI version |
| 32 | 2 | service JP-table file offset, or zero |
| 34 | 2 | service JP-table entry count |
| 36 | 2 | optional runtime JP-table code offset |
| 38 | 2 | runtime JP count: zero or four |
| 40 | 16 | NUL-padded image name (15 data bytes maximum) |
| 56 | 8 | reserved, zero |

The ordered service table immediately follows the descriptor. Every entry is
three bytes: Z80 opcode `0xc3` (`JP nn`) followed by a 16-bit XL code offset.
The complete, unmodified XL file follows all metadata. `xprog` accepts only
XL version 2, whose 12-byte header is followed by the code and then by the
relocation table (see the [xld README](../xld/README.md)); it checks the
version, the total length, the entry offset and every relocation record
before packaging.

If `--id` is omitted, the ID is the 32-bit FNV-1a hash of the image name. It is
descriptive metadata in this version; xprog does not define a dynamic-linking
or service-discovery contract.

## Loader interpretation

For a process, the preferred address refers to the relocated XL code base. For
a service, it refers to the first resident JP slot. A loader can place that
table at the preferred address, place and relocate the XL code after it, and
add the actual code base to every JP target offset. Thus a service requested at
`0xfd00` exposes slot 0 at `0xfd00`, slot 1 at `0xfd03`, and so on, while its
implementation remains relocatable.

Because the XL relocation table trails the code, a loader that reads the
payload sequentially receives the resident code before the table. It can
allocate the code block first, read the table into a temporary block that
sits above it, patch, and free only the table; YOS does the equivalent in
one read buffer by splitting the consumed table off the end of the block.

That describes only how to load one service image. How a separately linked
process discovers a service and binds calls to its slots remains intentionally
unspecified.

## Compile relocatable YOS images

`xcc --prc app.c --stack-size 512 -o app.prc` compiles, links XL and
packages it without discarding relocations. `--prc` and `--svc` select the
YOS target, default to minimum OS ABI 1, and require its `sdcccall(1)` ABI. Metadata switches accept a
separate value or `=value`: `--name`, `--id`, `--abi`, and `--min-os`.

`xcc --svc service.c --entry init --export function -o service.svc`
initializes C storage and the YOS libc binding before calling `init`.
The optional initializer receives the bound export table in HL and returns
zero on success in DE. Omit `--entry` for a service needing only C storage
initialization. Repeat `--export` in stable ABI slot order. Use
`-nostartfiles` only when supplying the complete assembly startup yourself;
then `--entry` addresses that startup directly.

`xprog --map app.map` resolves `--entry` and `--export` symbols from an xld
map; existing numeric offsets remain supported. Names match exactly first,
then with the C leading underscore. Unknown, ambiguous and out-of-range
symbols fail. A caller's `-Map=file` is retained by `xcc`.

Run `make -C x/src/xprog test-images` with the staged compiler available for
C/assembly compilation, metadata, rejection, CRC, relocation and execution
checks. One service image executes at three load addresses, with poisoned
BSS and stack-balance assertions.

## Loader-bound YOS call stubs

`--runtime-gates __yos_gate18 --map app.map` marks four consecutive JP
instructions whose default targets are 0018h, 0020h, 0028h and 0030h. The tool
validates all twelve bytes and records their code offset in header bytes 36–37
and the count (four) in bytes 38–39. Zero in all four header bytes means the
older unbound ABI. These entries must not be XL-relocated. Ordinary kernels
retain their defaults; a fixed-RAM kernel hidden by an expansion ROM can bind
them to equivalent always-visible service/far-call/far-data entries.

The YOS platform runtime and compiler use the shared `__yos_gate18/20/28/30`
stubs. Rebuild old objects that directly execute the restart instructions
before targeting an expansion that owns those vectors.
