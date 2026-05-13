# xdbg VSIX

This project provides a minimal Visual Studio Code debugger extension for
`xdbg`.

It does not implement a debug adapter itself. Instead, it launches:

```text
xdbg --interpreter=dap --quiet
```

and lets VS Code speak DAP directly with `xdbg` over stdio.

## Layout

- `package.json` declares the `xdbg` debugger type.
- `extension.js` starts `xdbg` as the debug adapter executable.
- `.vscode/launch.json` launches an Extension Development Host against the
  `tests/debug` sieve sample workspace.

## Debug Configuration Fields

- `program`: Target binary image.
- `symbols`: `.xdbg` symbols file.
- `remoteTarget`: Remote target in `host:port` format.
- `debuggerPath`: Path to the `xdbg` executable.
- `cwd`: Working directory used to launch `xdbg`.
- `stopOnEntry`: Stop immediately after launch or attach.

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

That produces:

- `tools/xdbg-vsix/xdbg-vsix-0.0.1.vsix`

You can still run the extension directly in VS Code using the included
Extension Development Host launch config.
