# Amstrad CPC targets

X Tools provides three firmware-hosted Amstrad CPC targets:

| Target | Machine | Delivery | File backend |
|---|---|---|---|
| `cpc-464` | CPC 464, 64 KiB | CDT cassette | Console descriptors only |
| `cpc-664` | CPC 664, 64 KiB | AMSDOS DSK | Firmware disk streams |
| `cpc-6128` | CPC 6128, 128 KiB | AMSDOS DSK | Firmware disk streams |

All three link at `0x4000`, use the firmware Text VDU, keyboard manager, and
300 Hz clock, and return to BASIC when `main()` returns. The linked image and
heap occupy lower RAM below `0x9F00`; a private C stack grows down immediately
below the AMSDOS workspace.

## Build a cassette program

```sh
mkdir -p build/examples/cpc-464
bin/x/bin/xcc -Os --platform=cpc-464 --oformat=binary \
  x/examples/cpc-464/hello.c -o build/examples/cpc-464/hello.bin
bin/x/bin/xprog --cdt build/examples/cpc-464/hello.bin \
  --name HELLO -o build/examples/cpc-464/hello.cdt
```

Insert `hello.cdt`, enter `RUN"!HELLO"`, and start the tape. The `!` suppresses
the physical PLAY prompt when an emulator has already started playback.
`xprog --cdt` writes CPC firmware binary headers and 2 KiB data blocks in a
CDT/TZX 1.20 container. The default load and entry address is `0x4000`, and
cassette names may contain up to 16 printable bytes.

## Build an AMSDOS disk program

Select either disk-equipped model:

```sh
mkdir -p build/examples/cpc-6128
bin/x/bin/xcc -Os --platform=cpc-6128 --oformat=binary \
  x/examples/cpc-6128/files.c -o build/examples/cpc-6128/files.bin
bin/x/bin/xprog --dsk build/examples/cpc-6128/files.bin \
  --name FILES.BIN -o build/examples/cpc-6128/files.dsk
```

For a CPC 664, replace both `cpc-6128` occurrences with `cpc-664`. Insert the
disk and enter:

```text
|DISC
RUN"FILES"
```

The CRT selects AMSDOS disk input and output itself, so `|DISC` is normally
redundant; it is useful when returning to the BASIC prompt after tape work.
`xprog --dsk` writes a standard CPCEMU DSK containing a 40-track, single-sided
AMSDOS data disk. It creates one 8.3 binary file with a valid AMSDOS header,
load address, entry address, logical length, and checksum.

## Library and file contract

The common archive-granular C library is available on every model. The 464
backend supplies console, keyboard, clock, heap, and process hooks; disk calls
fail without linking AMSDOS state or its two 2 KiB buffers, keeping cassette
programs smaller.

The 664 and 6128 backends expose the firmware's one input and one output
channel as descriptors 3 and 4. The usual C interfaces work on those streams:

- `open`, `read`, `write`, `close`, input `lseek`, `unlink`, and `rename`;
- `fopen`, `fread`, `fwrite`, `fclose`, `fseek`, `ftell`, and `remove`;
- console input on descriptor 0 and output on descriptors 1 and 2.

AMSDOS output is sequential. `O_RDWR`, append mode, and output seeking fail
explicitly rather than pretending to provide random access. Input seeking is
implemented through the ROM by reopening and skipping; headerless ASCII files
whose open result has no length are measured on the first `SEEK_END` or EOF.
Only one C input and one C output file can be open at a time, matching the
firmware channel model. File names are limited to the AMSDOS 16-byte firmware
input, including any drive or user prefix.

## Installable package

The repository package pass includes all three CPC definitions in the native
X toolchain package:

```sh
make packages
sudo dpkg -i bin/x/pkg/deb/x_*.deb
```

For each model the archive contains the assembled CRT, CRT source, GNU and
SDCC linker scripts, and platform library below `/opt/x/z80/lib/`. It also
contains this guide and the CDT/DSK-capable `xprog`. The package build extracts
the finished `.deb` and verifies those files, their ownership/modes, and both
CPC media switches. Run `make -C x/pkg/debian check` to repeat that audit.

## Emulator scope

These targets call the real CPC firmware and, on disk models, the AMSDOS ROM.
The repository's generic `xemu` currently does not implement a complete CPC
machine, Gate Array/ROM paging, or AMSDOS environment. Its Partner-style and
ZX-128-style banking examples demonstrate the generic memory mapper; they are
not CPC machine profiles. The CPC linker scripts keep the program in the
primary visible RAM below `0x9F00` and do not depend on the 6128's extra bank.
Use the real-ROM MCP regression below for machine-level acceptance.

## MCP regression

The real-ROM regression uses an `amstrad-cpc-mcp` checkout beside this
repository by default, or accepts explicit paths:

```sh
python3 x/tests/tests/cpc/run_mcp.py \
  --mcp /path/to/amstrad-cpc-mcp \
  --roms /path/to/rom-directory
```

It boots the generated CDT on a 464 and independent generated DSK images on a
664 and 6128. It checks startup/static data, the heap and common libc, console
polling and blocking input, the firmware clock, raw and stdio disk operations,
headerless-file seeking, rename/remove success and errors, and clean BASIC
return behavior. A successful run reports one `PASS` line for each model.
