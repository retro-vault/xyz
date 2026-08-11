# xprog — XL process and service image packager

## Synopsis

```text
xprog --process [options] input.xl [-o output.prc]
xprog --service [options] input.xl [-o output.svc]
xprog --inspect input
```

`xprog` validates an XL version 1 input and prefixes it with an XPRG version 1
descriptor. A process descriptor records its entry and required stack size. A
service descriptor records its preferred resident address and an ordered table
of Z80 `JP` entries. The XL payload remains relocatable.

## Options

`-p`, `--process`
: Create a `.prc` process image.

`-s`, `--service`
: Create a `.svc` service image.

`-i`, `--inspect`
: Validate and describe an existing XPRG image.

`-o file`
: Select the output. Otherwise the input extension becomes `.prc` or `.svc`.

`-n name`, `--name name`
: Set the image name (1–15 bytes). The input stem is the default.

`--id number`
: Set the 32-bit image ID. The default is FNV-1a of the image name.

`--abi number`
: Set the provided image ABI version. The default is 1.

`--min-os number`
: Set the minimum required OS ABI version.

`--load-address number`
: Set the preferred process code address or service JP-table address.

`--fixed-load`
: Mark the preferred address as required rather than advisory.

`--entry number`
: Override the XL code-relative process entry or specify a service initializer.

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
```

At service address `0xfd00`, ordered three-byte entries occupy `0xfd00`,
`0xfd03`, and so on. The service XL implementation can still be relocated by
the loader. Dynamic binding between separate process and service builds is not
defined by xprog version 1.
