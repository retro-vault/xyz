# xtools — Z80 toolchain

A complete C toolchain for the Z80: compiler, assembler, linker,
standalone optimizer, archiver, object converter, and source-level
debugger.

## Installation

The directory this README sits in is a self-contained, relocatable
install prefix. Copy it anywhere and add `bin/` to your PATH:

```bash
sudo cp -R . /opt/xtools
echo 'export PATH=/opt/xtools/bin:$PATH' >> ~/.bashrc
```

No environment variables or configuration files are needed — the tools
find their headers, runtime, and libraries relative to their own
location.

When building from the source checkout, install the generated Debian package:

```bash
make packages
sudo dpkg -i bin/x/pkg/deb/x_*.deb
```

The package installs under `/opt/x` by default. Nested `.deb` and `.vsix`
artifacts are deliberately not copied into the installed prefix.

## Quick start

```bash
# Compile, assemble, and link in one step
xcc hello.c -o hello.xl

# Several files, with optimization
xcc -Os main.c util.c -o app.xl

# Flat binary at a fixed address
xcc main.c --oformat=binary -Ttext=0x8000 -o app.bin

# ZX Spectrum tape and replacement ROM
xcc -Os --platform=zx-ram --oformat=binary main.c -o app.bin
xprog --tap app.bin -o app.tap --name APP
xcc -Os --platform=zx-rom --oformat=binary main.c -o app.rom

# ZX Spectrum esxDOS disk application, loaded into RAM or booted from ROM
xcc -Os --platform=zx-esxdos --oformat=binary main.c -o disk.bin
xcc -Os --platform=zx-esxdos-rom --oformat=binary main.c -o disk.rom

# Amstrad CPC cassette and AMSDOS disk
xcc -Os --platform=cpc-464 --oformat=binary main.c -o app.bin
xprog --cdt app.bin -o app.cdt --name APP
xcc -Os --platform=cpc-6128 --oformat=binary main.c -o app.bin
xprog --dsk app.bin -o app.dsk --name APP.BIN

# Debug a program
xcc -g main.c -o app.xl
xemu --listen 127.0.0.1:9000 &
xgdb --exec app.xl --cdb app.cdb --remote 127.0.0.1:9000
```

## The tools

| Tool | Purpose | Manual |
|---|---|---|
| `xcc` | C23-oriented compiler driver (GNU-style: drives xas and xld) | `share/doc/XCC.md` |
| `xas` | Assembler (SDCC and GNU dialects) | `share/doc/XAS.md` |
| `xld` | Linker (XL, flat binary, Intel HEX, ELF output) | `share/doc/XLD.md` |
| `xopt` | Standalone Z80 assembly optimizer | `share/doc/XOPT.md` |
| `xar` | Static library archiver | `share/doc/XAR.md` |
| `xobjcopy` | Object/archive format conversion | `share/doc/XOBJCOPY.md` |
| `xprog` | XPRG, TAP/TZX/CDT, and CPC DSK packager | `share/doc/XPROG.md` |
| `xgdb` | Source-level debugger | `share/doc/XGDB.md` |
| `xemu` | Standalone Z80 emulator and remote debug target | `share/doc/XEMU.md` |

`share/doc/ZX48.md` describes the installed Spectrum targets. The plain
`zx-ram` and `zx-rom` forms provide console services. `share/doc/ZX-ESXDOS.md`
documents disk calls for a booted esxDOS RAM application, while
`share/doc/ZX-ESXDOS-ROM.md` describes a disk application that boots and
executes from its own 16 KiB base ROM without requiring a Sinclair ROM,
using 48 bytes of RAM gates for firmware calls. Writable storage begins at
`0x5B00`; esxDOS's internal disk buffers occupy divIDE RAM. Both disk guides
describe the configurable stack allowance and post-entry low-RAM use.

`share/doc/CPC.md` documents the installed CPC 464/664/6128 targets, CDT/DSK
creation, and the AMSDOS stream contract.

## Prefix layout

```text
bin/          the tools
lib/          host SDK libraries (libxbfd, libxopt, librsp, libxgdb, libxemu, libxz80)
include/      host SDK headers (xbfd/, xopt/, rsp/, xgdb/, xemu/, xz80/)
z80/include/  common C headers and selected-platform header subdirectories
z80/lib/      crt0, linker scripts, libc, runtime, platform libraries
share/doc/    tool manuals
pkg/          installable packages (.deb, .vsix)
```

The default target platform is bare-metal `none` (`libnone.a`). Named staged
platforms include CP/M 3, CPC 464/664/6128, and ZX Spectrum 48K RAM/ROM forms
with or without esxDOS disk support; select one with `--platform=<name>`.
