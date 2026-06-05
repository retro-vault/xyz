# Distribution Layout

This directory contains the staged build output for `xyz`.

It is arranged so selected subdirectories can be copied directly into a
system prefix such as `/usr`:

- `bin/` for host-side executables
- `include/` for public headers, with Z80 target headers under `include/z80/`
- `lib/` for host-side static libraries, with Z80 target libraries under
  `lib/z80/`
- `libexec/xcc/` for compiler-private runtime and include support
- `share/xtools/` for staged documentation, examples, and templates
- `pkg/` for generated package artifacts

It also contains target-specific output that is not part of a normal
host `/usr` layout:

- `z80/` for generated generic and platform-specific target deliverables

## Top-Level Contents

### `bin/bin`

Host-side command-line programs:

- `appmake` converts tape and snapshot input into application payloads
- `mdr` compatibility alias for `microdrive`
- `microdrive` creates and edits `.mdr` cartridge images
- `serial` transfers data over a serial link
- `xas` assembler for Z80 build products
- `xar` archive tool for `.rel` libraries
- `xcc` C compiler for Z80 targets
- `xgdb` debugger frontend
- `xgdb-z80` local Z80 debug target
- `xld` linker for Z80 build products

### `bin/include`

Public host-side library headers staged from the repository root `include/`
tree and library public include trees.

Current host-side headers include:

- `microdrive/` for the microdrive library API
- `rsp/` for the remote-serial-protocol library API
- `xbfd/` for the binary/debug-info library API
- `xgdb/` for the debugger library API

### `bin/include/z80`

Public target-side headers for Z80 programs.

Current Z80 headers include:

- the staged C23 libc work under `lib/libc/include/`
- `platform/yos.h` for the YOS platform interface

### `bin/lib`

Host-side static libraries:

- `libmicrodrive.a`
- `librsp.a`
- `libxbfd.a`
- `libxgdb.a`
- `libxgdb_cli.a`
- `libxgdb_mi.a`

### `bin/lib/z80`

Target-side Z80 libraries:

- `libc.a` for the current standard C library work

### `bin/lib/z80/spectrum`

Reserved for ZX Spectrum-specific target libraries such as `crt0` or platform
overrides.

### `bin/libexec/xcc`

Compiler-private support files used by the Z80 toolchain:

- `include/` as a compatibility mirror of the staged Z80 libc headers
- `runtime/` for assembled `.rel` helpers and `z80.lib` / `runtime.lib`

### `bin/share/xtools/docs`

Mirrored project documentation staged from the repository `docs/` tree.
The staged layout keeps the same categories:

- `README.md` as the documentation index
- `dist/` for distribution-layout documentation
- `howtos/` for workflow guides
- `research/` for imported reference material
- `standards/` for coding and assembly standards
- `todo/` for current gap analyses
- `components/` for staged README files owned by individual tools and
  subprojects

Current staged component documents include:

- `components/APPMAKE.md`
- `components/MICRODRIVE.md`
- `components/SERIAL.md`
- `components/XLD.md`
- `components/YOS.md`

### `bin/z80/spectrum`

ZX Spectrum target deliverables:

- `bin/` for ROM images, staged application payloads, and `.mdr` media
- `include/` reserved for extra platform headers
- `lib/` reserved for extra platform libraries

### `bin/pkg`

Generated package artifacts:

- `deb/` for Debian package outputs
- `vsix/` for staged VS Code extension packages
