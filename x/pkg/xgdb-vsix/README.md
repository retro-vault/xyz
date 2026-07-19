# xgdb VSIX

This directory contains the Visual Studio Code extension scaffolding for
`xgdb`.

## Current Status

This VSIX launches `xgdb --interpreter=dap --quiet` as a native Debug Adapter
Protocol server. `xgdb` then connects to the target through GDB Remote Serial
Protocol, usually `xemu --listen HOST:PORT`.

The supported flow is:

```text
VS Code / xgdb-vsix
  <-> DAP
xgdb
  <-> RSP
xemu or hardware target
```

Native DAP smoke tests cover source breakpoints and stack frames for both C
and assembly files.

## Layout

- `package.json` declares the `xgdb` debugger type and package metadata
- `extension.js` launches `xgdb` as the adapter executable
- `.vscode/launch.json` is for extension-development testing

## Current Manifest Fields

The extension manifest currently defines these configuration fields:

- `program`
- `symbols`
- `remoteTarget`
- `origin`
- `pc`
- `startAddress`
- `noLoad`
- `debuggerPath`
- `cwd`
- `stopOnEntry`
- `cdbFile`
- `mapFile`
- `sourceRoot`
- `sourceRoots`
- `includeRoots`

When `program` is an ELF file, `xgdb` loads embedded symbols/debug
information automatically. Use `cdbFile` and `mapFile` for raw image or
sidecar-debug flows.

On launch, `xgdb` downloads `program` to the RSP target. ELF sections are
written to their linked VMAs and PC is set to the ELF entry unless `pc` or
`startAddress` is supplied. XL files are relocated at `origin` and default PC
to `origin + entry_point`; CDB/ELF/MAP symbol addresses are biased by the same
origin for source-level debugging. Raw binaries also use `origin`, defaulting
to `0x0000`. Set `noLoad` for an already-loaded target.

## Local Validation

Syntax-check the extension:

```sh
npm run check
```

## Packaging

Package the extension from this directory with the local packager:

```sh
npm run package
```

That produces an intermediate build artifact under:

- `build/pkg/xgdb-vsix/xgdb-vsix-<version>.vsix`

and stages the final package at:

- `bin/x/pkg/vsix/xgdb-vsix-<version>.vsix`

The VSIX version comes from `VSIX_VERSION` in the repository root
`Makefile`, which defaults to `PACKAGE_VERSION`.
