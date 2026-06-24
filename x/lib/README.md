# Libraries

This directory contains the reusable libraries used by the host-side tools
and the target toolchain.

## Current Libraries

- `rsp/`
  GDB Remote Serial Protocol transport used by `xgdb` and `xemu`
- `xgdb/`
  debugger-side document model and disassembly library
- `xemu/`
  reusable Z80 emulator library plus RSP target/session helpers
- `xbfd/`
  object, binary, and debug-info reader/writer support
- `xz80/`
  Z80 CPU and disassembly support library with tests
- `../libc/`
  target-side C library headers and archive for Z80 programs

Public headers live in library-local include trees such as
`<name>/include/` and are staged into:

- `bin/x/include/` for host-side libraries
- `bin/x/z80/include/` for target-side libc headers

## Build

Build all libraries:

```sh
make -C x/lib
```

Useful focused commands:

```sh
make -C x/lib/xgdb test
make -C x/lib/xemu test
make -C x/lib/xz80 test
make -C x/libc
```

## Linked Docs

- [librsp](rsp/README.md)
- [libxgdb](xgdb/README.md)
- [libxemu](xemu/README.md)
