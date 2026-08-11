# xprog

`xprog` validates an XL image and prefixes it with the XPRG version 1
descriptor. It creates either a `.prc` process image or a `.svc` resident
service image. It does not link programs, resolve symbols, or install either
kind of image.

## Usage

```sh
xprog --process app.xl --stack-size 1024
xprog --service runtime.xl --load-address 0xfd00 --fixed-load \
    --export 0x0010 --export 0x0038
xprog --inspect app.prc
```

The default output name replaces `.xl` with `.prc` or `.svc`. Use `-o` to
choose another path. Numbers accept decimal or a `0x` hexadecimal prefix.
Running `xprog` without arguments prints the complete usage text.

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
| 36 | 4 | reserved, zero |
| 40 | 16 | NUL-padded image name (15 data bytes maximum) |
| 56 | 8 | reserved, zero |

The ordered service table immediately follows the descriptor. Every entry is
three bytes: Z80 opcode `0xc3` (`JP nn`) followed by a 16-bit XL code offset.
The complete, unmodified XL file follows all metadata.

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

That describes only how to load one service image. How a separately linked
process discovers a service and binds calls to its slots remains intentionally
unspecified.
