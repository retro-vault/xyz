# Libraries

This directory contains the reusable libraries used by the host-side tools
and the target toolchain.

## Current Libraries

- `lib/rsp/`
  GDB Remote Serial Protocol transport used by `xgdb` and `xgdb-z80`
- `lib/xgdb/`
  debugger-side document model and disassembly library
- `lib/xbfd/`
  object, binary, and debug-info reader/writer support
- `lib/xz80/`
  Z80 CPU and disassembly support library with tests
- `lib/libc/`
  target-side C library headers and archive for Z80 programs

Public headers live in library-local include trees such as
`lib/<name>/include/` and are staged into:

- `bin/include/` for host-side libraries
- `bin/include/z80/` for target-side libc headers

## Build

Build all libraries:

```sh
make -C lib
```

Useful focused commands:

```sh
make -C lib/xgdb test
make -C lib/xz80 test
make -C lib/libc
```

## Linked Docs

- [librsp](rsp/README.md)
- [libxgdb](xgdb/README.md)
