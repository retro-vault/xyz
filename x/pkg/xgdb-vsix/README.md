# xgdb VSIX

This directory contains the Visual Studio Code extension scaffolding for
`xgdb`.

## Current Status

This is not yet a supported end-to-end debugger workflow.

What exists today:

- a VS Code extension manifest that contributes the `xgdb` debugger type
- an adapter launcher in `extension.js`
- local packaging and staging into `bin/pkg/vsix/`

What is not aligned yet:

- the extension launcher currently tries `xgdb --interpreter=dap --quiet`
- the current `xgdb` command-line entry point only exposes `cli` and `mi`
- the extension manifest still models a `symbols` / `.xgdb` input, while
  current debugger flows are centered on `.cdb` plus optional `.map`

So the honest state is:

- packaging the VSIX works
- the extension source is a useful starting point
- native F5 debugging through this VSIX is still in progress

## Layout

- `package.json` declares the `xgdb` debugger type and package metadata
- `extension.js` launches `xgdb` as the adapter executable
- `.vscode/launch.json` is for extension-development testing

## Current Manifest Fields

The extension manifest currently defines these configuration fields:

- `program`
- `symbols`
- `remoteTarget`
- `debuggerPath`
- `cwd`
- `stopOnEntry`

Treat that schema as provisional until the debugger-side DAP path is wired
through the live `xgdb` binary.

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
