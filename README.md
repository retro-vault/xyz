# xyz

`xyz` is a ZX Spectrum systems project with three main threads:

- `xc`: a Z80-oriented compiler, assembler, linker, debugger, and support
  tooling stack
- `yos`: a ROM-based operating system for the Sinclair ZX Spectrum
- `zwin`: an early placeholder for a future windowing layer

The repository also contains host-side tools, debugger libraries, staged
distribution packaging, and regression tests that tie the whole toolchain
together.

This README is intentionally short. Use it as the landing page, then jump
to the specific documentation you need.

## Start Here

- [Project documentation map](docs/README.md)
- [Distribution layout](docs/dist/README.md)
- [How to test](docs/howtos/HOW-TO-TEST.md)
- [Debugger integration](docs/howtos/DEBUGGER_INTEGRATION.md)
- [Current libc gaps](docs/todo/LIBC-GAPS.md)

## Standards

- [C++ coding style](docs/standards/CPP-CODING-STYLE.md)
- [General Z80 coding style](docs/standards/Z80-CODING-STYLE.md)
- [YOS assembly style guide](docs/standards/YOS-ASSEMBLY_STYLE_GUIDE.md)

## Core Components

- [Compiler suite overview](src/xc/README.md)
- [YOS overview](src/yos/README.md)
- [YOS chapter index](src/yos/INDEX.md)
- [zwin status](src/zwin/README.md)

## Libraries

- [Libraries overview](lib/README.md)
- [librsp](lib/rsp/README.md)
- [libxgdb](lib/xgdb/README.md)

## Host Tools

- [appmake](tools/appmake/README.md)
- [microdrive](tools/microdrive/README.md)
- [serial](tools/serial/README.md)
- [xgdb VS Code extension](tools/xgdb-vsix/README.md)

## Tests And Samples

- [Debug sample](tests/debug/README.md)
- [Microdrive emulator harness](tests/mdr-emu/README.md)

## Staged Output

- [Staged `bin/` layout](bin/README.md)
