# xprog — XPRG and ZX Spectrum tape packager

## Synopsis

```text
xprog --process [options] input.xl [-o output.prc]
xprog --service [options] input.xl [-o output.svc]
xprog --inspect input
xprog --tap [options] input.bin [-o output.tap]
xprog --tzx [options] input.bin [-o output.tzx]
```

`xprog` validates an XL version 1 input and prefixes it with an XPRG version 1
descriptor. A process descriptor records its entry and required stack size. A
service descriptor records its preferred resident address and an ordered table
of Z80 `JP` entries. The XL payload remains relocatable.

The tape modes wrap a flat binary in an auto-running ZX Spectrum 48K image.
The default load and entry address is `0x5CCB`.

## Options

`-p`, `--process`
: Create a `.prc` process image.

`-s`, `--service`
: Create a `.svc` service image.

`-i`, `--inspect`
: Validate and describe an existing XPRG image.

`--tap`
: Create a checksummed TAP with a BASIC bootstrap and CODE block.

`--tzx`
: Create a TZX 1.20 image containing the same four standard-speed blocks.

`-o file`
: Select the output. Otherwise the extension follows the selected mode.

`-n name`, `--name name`
: Set the image name. XPRG names allow 15 bytes and tape headers allow 10.
  The input stem is the default.

`--id number`
: Set the 32-bit image ID. The default is FNV-1a of the image name.

`--abi number`
: Set the provided image ABI version. The default is 1.

`--min-os number`
: Set the minimum required OS ABI version.

`--load-address number`
: Set the preferred process code address, service JP-table address, or tape
  CODE load address. Tape output defaults to `0x5CCB`.

`--fixed-load`
: Mark the preferred address as required rather than advisory.

`--entry number`
: Override the XL code-relative process entry, specify a service initializer,
  or select the absolute tape entry address.

`--stack-size number`
: Set the required nonzero process stack size. Required in process mode.

`--export offset`
: Append a service JP slot targeting an XL code offset. Repeat in stable ABI
  slot order. At least one is required in service mode.

`--version`
: Print the tool version.

`-h`, `--help`
: Print complete usage.

Numbers may be decimal or hexadecimal with a `0x` prefix.

## Examples

```sh
xprog --process shell.xl --stack-size 0x400
xprog --service runtime.xl --load-address 0xfd00 --fixed-load \
    --export 0x20 --export 0x48
xprog --inspect runtime.svc
xprog --tap hello.bin --name HELLO
xprog --tzx hello.bin --load-address 0x8000 --entry 0x8010
```

Tape output contains a program header/data pair followed by a CODE header/data
pair. The auto-start BASIC program invokes a 30-byte loader held after a `REM`
token. That loader uses the standard ROM tape routine and returns directly to
the selected entry, so a CODE block starting at `0x5CCB` may overwrite the
BASIC program safely. This is why the default is the first byte after the 48K
ROM system variables rather than a conventional higher address.

Every block carries the standard Spectrum XOR checksum. TZX output is version
1.20 and represents the same four blocks as standard-speed data with a
one-second pause after each block. Tape mode rejects an empty binary, a binary
that crosses `0xFFFF`, an entry outside the loaded range, a zero load address,
or a name outside the 1–10 byte Spectrum header limit.

At service address `0xfd00`, ordered three-byte entries occupy `0xfd00`,
`0xfd03`, and so on. The service XL implementation can still be relocated by
the loader. Dynamic binding between separate process and service builds is not
defined by xprog version 1.
