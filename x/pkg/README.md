# X package build and verification

X owns two optional installable artifacts:

- the native Debian toolchain package under `bin/x/pkg/deb/`;
- the XGDB Visual Studio Code extension under `bin/x/pkg/vsix/`.

Build both from the repository root:

```sh
make packages
```

The root target delegates to `x/pkg/`. The Debian package defaults to the
medium X model and installs a relocatable toolchain below `/opt/x`; override
the usual `X_MODEL`, `PACKAGE_NAME`, `PACKAGE_VERSION`, `PACKAGE_RELEASE`, or
`INSTALL_PREFIX` Make variables when a different release is required.

## Packaged target contract

The Debian package contains the common Z80 headers, libc, fixed-point and
runtime archives, plus complete installed definitions for these targets:

```text
none
cpm3
cpc-464
cpc-664
cpc-6128
zx-ram
zx-rom
```

Each target definition includes `crt0-<target>.rel`, its `crt0` assembly
source, `linker-<target>.ld`, `linker-<target>.lk`, and
`lib<target>.a`. The CPC package surface also includes `CPC.md` and an
`xprog` with `--cdt` and `--dsk` modes.

The host-only `emu` target is staged for repository tests but deliberately
removed from the installable target sysroot. The `xemu` executable and its
host libraries remain in the package.

## Verification

`make packages` does not stop after creating the `.deb`. It extracts the
finished archive and checks:

- `root:root` archive ownership and normalized executable/data modes;
- required host tools, target headers, common libraries, and manuals;
- every packaged CRT object/source, linker script, and platform archive;
- the installed `xprog --cdt` and `xprog --dsk` command surface.

Repeat only the Debian archive verification with:

```sh
make -C x/pkg/debian check
```

The built-in package check is host-only because CPC ROMs are external assets.
For hardware-level release acceptance, extract or install the package and pass
its `xcc` and `xprog` paths to `x/tests/tests/cpc/run_mcp.py`. Validate CPC 464
CDT and CPC 664/6128 DSK delivery through that installed-package flow.
