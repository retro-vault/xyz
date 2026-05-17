# Distribution Layout

This directory contains the staged build output for `xyz`.

It is arranged so selected subdirectories can be copied directly into a
system prefix such as `/usr`:

- `bin/` for host-side executables
- `docs/` for packaged component documentation
- `include/` for public headers
- `lib/` for static libraries

It also contains target-specific output that is not part of a normal
host `/usr` layout:

- `targets/` for generated ZX Spectrum and generic Z80 build artifacts
- `extensions/` for editor integrations such as VS Code packages

## Top-Level Contents

### `bin/bin`

Host-side command-line programs:

- `appmake` converts tape and snapshot input into application payloads
- `mdr` compatibility alias for `microdrive`
- `microdrive` creates and edits `.mdr` cartridge images
- `serial` transfers data over a serial link
- `xdbg` debugger frontend
- `xdbg-z80` local Z80 debug target
- `xlink` linker for Z80 build products

### `bin/include`

Public headers staged from the repository root `include/` tree.

Current public headers include:

- `yos.h` for the YOS interface
- `microdrive/` for the microdrive library API
- `xdbg/` for the debugger library API
- `xdbgstub/` for the debug stub library API

### `bin/docs`

Packaged component documentation staged with stable filenames:

- `APPMAKE.md`
- `DEBUGGER_INTEGRATION.md`
- `MICRODRIVE.md`
- `SERIAL.md`
- `XLINK.md`
- `YOS.md`

### `bin/lib`

Host-side static libraries:

- `libmicrodrive.a`
- `libxdbg.a`
- `libxdbg_cli.a`
- `libxdbg_dap.a`
- `libxdbg_mi.a`
- `libxdbgstub.a`

### `bin/extensions`

Editor integration artifacts.

Current intended location:

- `vscode/` for packaged VS Code extensions such as `.vsix` files

### `bin/targets`

Generated target-side artifacts grouped by machine family.

#### `bin/targets/zxspectrum`

ZX Spectrum outputs:

- `roms/` for ROM images such as `yos.rom`
- `apps/` for ZX Spectrum application payloads
- `mdr/` for microdrive cartridge images such as `hello.mdr`

#### `bin/targets/z80`

Generic Z80 outputs:

- `apps/` for non-ZX-Spectrum Z80 application/debug payloads such as the
  `tests/debug` `sieve` sample
