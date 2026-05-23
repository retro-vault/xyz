# Debug Sample

This directory contains a tiny standalone Z80 C program meant for
debugging with:

- `xlink`
- `xdbg`
- `xdbg-z80`

The sample is intentionally simple:

- no headers
- no libc
- no YOS services
- one C function, `sieve()`

It builds into:

- a flat binary linked by `xlink`
- a linked `.xdbg` sidecar emitted by `xlink`

## Files

[sieve.c](/home/tstih/data/retro-vault/xyz/tests/debug/sieve.c)

- the sample C program

[crt0.s](/home/tstih/data/retro-vault/xyz/tests/debug/crt0.s)

- tiny startup stub
- calls `_main`
- halts forever after `main` returns

## Build

Build the sample:

```sh
make -C tests/debug all
```

That produces:

- `bin/targets/zxspectrum/apps/debug/sieve.bin`
- `bin/targets/zxspectrum/apps/debug/sieve.xdbg`

The build path is:

1. `sdcc` compiles `sieve.c` to `sieve.rel`
2. `sdasz80` assembles `crt0.s` to `crt0.rel`
3. `xlink -g` links both objects into `sieve.bin` and `sieve.xdbg`

## Run The Reference Emulator

In one terminal:

```sh
bin/bin/xc/xdbg/xdbg-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/targets/zxspectrum/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

This loads the binary at `0x0100`, starts execution at `_entry`, and
provides a remote debug target on TCP port `9000`.

## Run The Debugger

In a second terminal:

```sh
bin/bin/xc/xdbg/xdbg \
    --exec bin/targets/zxspectrum/apps/debug/sieve.bin \
    --symbols bin/targets/zxspectrum/apps/debug/sieve.xdbg \
    --remote 127.0.0.1:9000
```

## Run It From DDD

You can also point GNU DDD at `xdbg` as a custom debugger command.

Start `xdbg-z80` first:

```sh
bin/bin/xc/xdbg/xdbg-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/targets/zxspectrum/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

Then, from the repo root, launch DDD like this:

```sh
ddd --debugger "bin/bin/xc/xdbg/xdbg --quiet --exec bin/targets/zxspectrum/apps/debug/sieve.bin --symbols bin/targets/zxspectrum/apps/debug/sieve.xdbg --remote 127.0.0.1:9000"
```

Once DDD opens:

1. Set a breakpoint on `sieve` using the command tool or the debugger
   console:

   ```text
   break _sieve
   ```

2. Continue execution:

   ```text
   continue
   ```

3. When the target stops, DDD should open
   [sieve.c](/home/tstih/data/retro-vault/xyz/tests/debug/sieve.c) and
   highlight the current C source line.

4. If the source window does not jump immediately, enter:

   ```text
   list
   ```

   That forces `xdbg` to emit source context, which helps DDD align the
   current location with the loaded `.xdbg` file.

Useful commands from the DDD command tool are the same as in terminal
`xdbg`:

- `break _sieve`
- `continue`
- `stepi`
- `info registers`
- `disassemble`
- `x/80xb _flags`

## DDD Compatibility Note

This flow is best-effort right now.

`xdbg` is intentionally GDB-like in switches and common commands, but it
is not yet a full GDB replacement:

- it does not implement GDB/MI
- it does not implement the GDB remote serial protocol
- DDD integration depends on the subset of command-line behavior DDD
  expects from the inferior debugger

So for now:

- terminal `xdbg` is the most reliable path
- DDD should be useful for viewing C source and driving common commands
- more advanced DDD features may not work until `xdbg` grows a fuller
  compatibility layer

## Run It From VS Code

This directory now contains ready-to-use VS Code files:

- [tests/debug/.vscode/tasks.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/tasks.json)
- [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)

Open `tests/debug` itself as the VS Code workspace folder.

There are now two VS Code workflows:

1. task-based terminal workflow
2. native F5 workflow through the `xdbg-vsix` DAP extension

### Native DAP Workflow

The repo now contains a dedicated VS Code extension project in:

- [tools/xdbg-vsix/package.json](/home/tstih/data/retro-vault/xyz/tools/xdbg-vsix/package.json)
- [tools/xdbg-vsix/extension.js](/home/tstih/data/retro-vault/xyz/tools/xdbg-vsix/extension.js)
- [tools/xdbg-vsix/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tools/xdbg-vsix/.vscode/launch.json)

This extension contributes a new debugger type named `xdbg` and starts:

```text
xdbg --interpreter=dap --quiet
```

The included [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)
now also contains:

```json
{
  "name": "xdbg sieve (dap)",
  "type": "xdbg",
  "request": "launch",
  "program": "${workspaceFolder}/../../bin/targets/zxspectrum/apps/debug/sieve.bin",
  "symbols": "${workspaceFolder}/../../bin/targets/zxspectrum/apps/debug/sieve.xdbg",
  "remoteTarget": "127.0.0.1:9000",
  "debuggerPath": "${workspaceFolder}/../../bin/bin/xc/xdbg/xdbg",
  "cwd": "${workspaceFolder}",
  "stopOnEntry": true
}
```

To test it:

1. Open `tools/xdbg-vsix` in VS Code.
2. Run the `Run xdbg VSIX on sieve` launch configuration.
3. Start your target separately so it is already listening on
   `127.0.0.1:9000`.
4. In the Extension Development Host window that opens, select
   `xdbg sieve (dap)` and press `F5`.
5. Open [sieve.c](/home/tstih/data/retro-vault/xyz/tests/debug/sieve.c),
   set breakpoints, then step and inspect state through the standard VS Code
   debug UI.

### Included `tasks.json`

The included `tasks.json` provides:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build sieve debug sample",
      "type": "shell",
      "command": "make -C tests/debug all",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "run xdbg-z80 sieve target",
      "type": "process",
      "command": "${workspaceFolder}/run_xdbg_z80.sh",
      "dependsOn": "build sieve debug sample",
      "isBackground": true,
      "problemMatcher": []
    },
    {
      "label": "stop xdbg-z80 sieve target",
      "type": "process",
      "command": "${workspaceFolder}/stop_xdbg_z80.sh",
      "problemMatcher": []
    }
  ]
}
```

### How To Use The Tasks

Inside VS Code:

1. Run `Tasks: Run Task`
2. Choose `run xdbg-z80 sieve target`
3. Open `tests/debug/sieve.c` in the editor

This is the target-only half of the workflow. Pair it with the DAP
launch configuration described above, or connect manually from another
terminal if you want to use raw `xdbg`.

The DAP launch configuration does not start or stop the target for you.
Use the provided tasks only if you want a manual convenience wrapper
around the reference `xdbg-z80` target.

### Included `launch.json`

The included `launch.json` is the native `xdbg` DAP configuration:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "xdbg sieve (dap)",
      "type": "xdbg",
      "request": "launch",
      "program": "${workspaceFolder}/../../bin/targets/zxspectrum/apps/debug/sieve.bin",
      "symbols": "${workspaceFolder}/../../bin/targets/zxspectrum/apps/debug/sieve.xdbg",
      "remoteTarget": "127.0.0.1:9000",
      "debuggerPath": "${workspaceFolder}/../../bin/bin/xc/xdbg/xdbg",
      "cwd": "${workspaceFolder}",
      "stopOnEntry": true
    }
  ]
}
```

### How To Try F5 Debugging

Inside VS Code:

1. Open the `tests/debug` folder
2. Open [sieve.c](/home/tstih/data/retro-vault/xyz/tests/debug/sieve.c)
3. Go to `Run and Debug`
4. Select `xdbg sieve (dap)`
5. Press `F5`

VS Code should then:

1. launch `xdbg` in DAP mode
2. connect to the already-running remote target
3. drive the session through the native VS Code debug UI

### What To Expect From The F5 Path

This is the preferred VS Code path now. The older `cppdbg` / MI sample
configuration has been removed from `tests/debug/.vscode` to avoid
accidentally selecting the wrong debugger backend.

### Best Current VS Code Layout

A practical layout is:

1. left editor pane: `tests/debug/sieve.c`
2. bottom terminal 1: `xdbg-z80`
3. bottom terminal 2: `xdbg`

Then when `xdbg` stops on `_sieve`, use:

```text
list
```

to print the active source context directly in the debugger terminal
while the same file is open in the editor.

### Future Direction

Once `xdbg` grows either:

- a fuller MI implementation, or
- a real UDAP adapter for VS Code

this README can be upgraded to a real `launch.json`-based F5 workflow.

## Useful Commands

Set a breakpoint at the C function:

```text
(xdbg) break _sieve
(xdbg) continue
```

Show current source:

```text
(xdbg) list
```

Show available functions:

```text
(xdbg) info functions
```

Show registers:

```text
(xdbg) info registers
```

Disassemble around the current PC:

```text
(xdbg) disassemble
```

Or from a specific symbol:

```text
(xdbg) x/12i _sieve
```

Inspect the sieve result globals in memory:

```text
(xdbg) x/4xb _prime_count
(xdbg) x/4xb _last_prime
(xdbg) x/80xb _flags
```

Single-step one instruction:

```text
(xdbg) stepi
```

Detach:

```text
(xdbg) detach
```

## What To Expect

When `sieve()` finishes:

- `_prime_count` should contain the number of primes from `2` to `64`
- `_last_prime` should contain the last prime in that range
- the `flags` array shows which values remained marked

Since the startup stub halts after `main` returns, continuing without a
breakpoint will eventually stop with a `halted` reason.
