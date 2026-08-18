# xyz

`xyz` is a ZX Spectrum systems project with three main product roots:

- `x`: a Z80-oriented compiler, assembler, linker, debugger, and support
  tooling stack
- `y`: a ROM-based operating system for the Sinclair ZX Spectrum
- `z`: an early placeholder for a future windowing layer

The repository also contains host-side tools, debugger libraries, staged
distribution packaging, and regression tests that tie the whole toolchain
together.

The active product roots are `x/`, `y/`, and `z/`. Packaging lives under
`x/pkg/` and `y/pkg/`. Component release notes now live with those roots.

This README is intentionally short. Use it as the landing page, then jump
to the specific documentation you need.

## Start Here

- [Project documentation map](x/docs/README.md)
- [X release notes](x/CHANGELOG.md)
- [Y release notes](y/CHANGELOG.md)
- [Z release notes](z/CHANGELOG.md)
- [X feature guide](x/FEATURES.md)
- [Distribution layout](x/docs/dist/README.md)
- [How to test](x/docs/howtos/HOW-TO-TEST.md)
- [ZX Spectrum 48K RAM/ROM target guide](x/docs/howtos/ZX-SPECTRUM-48K.md)
- [Debugger integration](x/docs/howtos/DEBUGGER_INTEGRATION.md)
- [Current libc gaps](x/docs/todo/LIBC-GAPS.md)

## Standards

- [C++ coding style](x/docs/standards/CPP-CODING-STYLE.md)
- [General Z80 coding style](x/docs/standards/Z80-CODING-STYLE.md)
- [YOS assembly style guide](y/docs/YOS-ASSEMBLY_STYLE_GUIDE.md)

## Core Components

- [X tools overview](x/README.md)
- [YOS overview](y/README.md)
- [Z status](z/README.md)

## Libraries

- [Libraries overview](x/lib/README.md)
- [librsp](x/lib/rsp/README.md)
- [libxgdb](x/lib/xgdb/README.md)

## Host Tools

- [appmake](y/pkg/appmake/README.md)
- [microdrive](y/pkg/microdrive/README.md)
- [serial](y/pkg/serial/README.md)
- [xgdb VS Code extension](x/pkg/xgdb-vsix/README.md)

## Tests And Samples

- [CP/M 3 example](x/examples/cpm3/README.md)
- [ZX Spectrum RAM example](x/examples/zx-ram/README.md)
- [ZX Spectrum ROM example](x/examples/zx-rom/README.md)
- [ZX Spectrum MCP smoke test](x/tests/tests/zx48/README.md)
- [Debug sample](x/tests/tests/debug/README.md)
- [Microdrive emulator harness](y/tests/mdr-emu/README.md)

## Staged Output

- [Staged `xtools` layout](x/docs/dist/README.md)
