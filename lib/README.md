# Libraries

This directory contains the host-side debugger libraries built in this
repo.

## Components

[lib/xdbg/README.md](/home/tstih/data/retro-vault/xyz/lib/xdbg/README.md)

- linked debug database model and `.xdbg` reader/writer
- pluggable disassembler interface
- Z80 decoder plus SDCC-style formatter

[lib/xdbgstub/README.md](/home/tstih/data/retro-vault/xyz/lib/xdbgstub/README.md)

- remote debugger transport library
- debugger-side client
- emulator-side server and target interface

## Build

Build both libraries:

```sh
make -C lib
```

Run their tests individually:

```sh
make -C lib/xdbg test
make -C lib/xdbgstub test
```
