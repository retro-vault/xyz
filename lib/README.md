# Libraries

This directory contains the host-side debugger libraries built in this
repo.

## Components

[lib/xgdb/README.md](/home/tstih/data/retro-vault/xyz/lib/xgdb/README.md)

- linked debug database model plus disassembly helpers
- pluggable disassembler interface
- Z80 decoder plus SDCC-style formatter

`lib/rsp`

- remote debugger transport library
- GDB remote serial protocol server and target interface
- shared transport used by `xgdb` and `xgdb-z80`

## Build

Build both libraries:

```sh
make -C lib
```

Run the `xgdb` library tests individually:

```sh
make -C lib/xgdb test
```
