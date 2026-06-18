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

Alternatively, install the Debian package from `pkg/deb/`:

```bash
sudo dpkg -i pkg/deb/xtools_*.deb
```

## Quick start

```bash
# Compile, assemble, and link in one step
xcc hello.c -o hello.xl

# Several files, with optimization
xcc -Os main.c util.c -o app.xl

# Flat binary at a fixed address
xcc main.c --oformat=binary -Ttext=0x8000 -o app.bin

# Debug a program
xcc -g main.c -o app.xl
xgdb-z80 --listen 127.0.0.1:9000 &
xgdb --exec app.xl --cdb app.cdb --remote 127.0.0.1:9000
```

## The tools

| Tool | Purpose | Manual |
|---|---|---|
| `xcc` | C11 compiler driver (GNU-style: drives xas and xld) | `share/doc/XCC.md` |
| `xas` | Assembler (SDCC and GNU dialects) | `share/doc/XAS.md` |
| `xld` | Linker (XL, flat binary, Intel HEX, ELF output) | `share/doc/XLD.md` |
| `xopt` | Standalone Z80 assembly optimizer | `share/doc/XOPT.md` |
| `xar` | Static library archiver | `share/doc/XAR.md` |
| `xobjcopy` | Object/archive format conversion | `share/doc/XOBJCOPY.md` |
| `xgdb`, `xgdb-z80` | Source-level debugger and Z80 gdbserver | `share/doc/XGDB.md` |

## Prefix layout

```text
bin/          the tools
lib/          host SDK libraries (libxbfd, libxopt, librsp, libxgdb)
include/      host SDK headers (xbfd/, xopt/, rsp/, xgdb/)
z80/include/  C library headers for the target
z80/lib/      crt0, linker scripts, libc, runtime, platform libraries
share/doc/    tool manuals
pkg/          installable packages (.deb, .vsix)
```

The default target platform is bare-metal `none` (`libnone.a`). CP/M 3
support is staged as `libcpm3.a`; select it with `--platform=cpm3`.
