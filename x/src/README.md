# xc

`xc` is the active Z80 toolchain in this repository.

Today it builds these host-side tools into `bin/x/bin/`:

- `xcc` — C compiler for Z80 targets
- `xas` — assembler plus assembly pretty-printer / dialect converter
- `xar` — archive tool for `.rel` libraries
- `xobjcopy` — object/archive conversion and debug-stripping tool
- `xld` — linker for `XL`, `BIN`, `CDB`, and related outputs
- `xgdb` — debugger frontend
- `xgdb-z80` — reference remote Z80 debug target

## Layout

- `xar/` — archive tool sources
- `xas/` — assembler sources and tests
- `xcc/` — compiler sources, optimizer, runtime helpers, and compiler docs
- `xobjcopy/` — object/archive conversion tool sources and tests
- `xgdb/` — debugger frontend and reference target
- `xld/` — linker sources, README, and tests

## Build

Build the full toolchain slice:

```sh
make -C x all
```

The wider repository build stages the results under:

- `bin/x/bin/` for host executables
- `bin/x/include/` and `bin/x/lib/` for host SDK headers and libraries
- `bin/x/z80/include/` and `bin/x/z80/lib/` for libc headers and the
  runtime, startup, libc, and platform archives
- `bin/y/` for YOS images
- `bin/z/` for staged target apps and media

## Where To Read Next

- [How to test the repo](../docs/howtos/HOW-TO-TEST.md)
- [xobjcopy README](xobjcopy/README.md)
- [xld README](xld/README.md)
- [xgdb README](xgdb/README.md)
- `xcc/docs/` for compiler internals and status notes
