# xgdb VSIX

This project provides a minimal Visual Studio Code debugger extension for
`xgdb`.

It does not implement a debug adapter itself. Instead, it launches:

```text
xgdb --interpreter=dap --quiet
```

and lets VS Code speak DAP directly with `xgdb` over stdio.

## Layout

- `package.json` declares the `xgdb` debugger type.
- `extension.js` starts `xgdb` as the debug adapter executable.
- `.vscode/launch.json` launches an Extension Development Host against the
  `tests/debug` sieve sample workspace.

## Debug Configuration Fields

- `program`: Target binary image.
- `symbols`: `.xgdb` symbols file.
- `remoteTarget`: Remote target in `host:port` format.
- `debuggerPath`: Path to the `xgdb` executable.
- `cwd`: Working directory used to launch `xgdb`.
- `stopOnEntry`: Stop immediately after launch or attach.

The extension does not start an emulator or target process for you.
Your configured `remoteTarget` is expected to already be listening.

Current debugger-side behavior worth knowing:

- if the linked `.xgdb` file has no real source file for a library
  function, `xgdb` will not invent one
- in that case VS Code may switch to disassembly-oriented navigation for
  that frame
- `xgdb` now supports the DAP `disassemble` request, so assembly fallback
  is available instead of failing source lookup outright

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

- `build/tools/xgdb-vsix/xgdb-vsix-<version>.vsix`

and stages the final package at:

- `bin/pkg/vsix/xgdb-vsix-<version>.vsix`

The VSIX version comes from `VSIX_VERSION` in the repository root
`Makefile`, which defaults to `PACKAGE_VERSION`.

You can still run the extension directly in VS Code using the included
Extension Development Host launch config.
