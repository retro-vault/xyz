# Debug Sample

This directory contains a tiny standalone Z80 C program meant for
debugging with the native X toolchain:

- `xcc`
- `xas`
- `xld`
- `xgdb`
- `xgdb-z80`

The sample is intentionally simple:

- no headers
- no libc
- no YOS services
- one C function, `sieve()`

It builds into:

- a flat binary linked by `xld`
- a linked `.cdb` sidecar emitted by `xld`

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

- `bin/z80/bin/apps/debug/sieve.bin`
- `bin/z80/bin/apps/debug/sieve.cdb`

The build path is:

1. `xcc -g -S` compiles `sieve.c` to `sieve.s` and emits `sieve.adb`
2. `xas --mode=sdcc` assembles `sieve.s` to `sieve.rel`
3. `xas --mode=sdcc` assembles `crt0.s` to `crt0.rel`
4. `xas --mode=sdcc` assembles the runtime helpers from `src/xc/xcc/lib/runtime/*.s`
5. `xar rcs` packs those helpers into `runtime.lib`
6. `xld -g` links the sample objects against `runtime.lib` into `sieve.bin` and `sieve.cdb`

## Run The Reference Emulator

In one terminal:

```sh
bin/bin/xgdb-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/z80/bin/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

This loads the binary at `0x0100`, starts execution at `_entry`, and
provides a remote debug target on TCP port `9000`.

## Run The Debugger

In a second terminal:

```sh
bin/bin/xgdb \
    --exec bin/z80/bin/apps/debug/sieve.bin \
    --cdb bin/z80/bin/apps/debug/sieve.cdb \
    --remote 127.0.0.1:9000 \
    -d tests/debug
```

## Run It From DDD

You can also point GNU DDD at `xgdb` as a custom debugger command.

Start `xgdb-z80` first:

```sh
bin/bin/xgdb-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/z80/bin/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

Then, from the repo root, launch DDD like this:

```sh
ddd --debugger "bin/bin/xgdb --quiet --exec bin/z80/bin/apps/debug/sieve.bin --cdb bin/z80/bin/apps/debug/sieve.cdb --remote 127.0.0.1:9000 -d tests/debug"
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

   That forces `xgdb` to emit source context, which helps DDD align the
   current location with the loaded `.cdb` file.

Useful commands from the DDD command tool are the same as in terminal
`xgdb`:

- `break _sieve`
- `continue`
- `stepi`
- `info registers`
- `disassemble`
- `x/80xb _flags`

## DDD Compatibility Note

This flow is best-effort right now.

`xgdb` is intentionally GDB-like in switches and common commands, but it
is not yet a full GDB replacement:

- it does not implement GDB/MI
- it does not implement the GDB remote serial protocol
- DDD integration depends on the subset of command-line behavior DDD
  expects from the inferior debugger

So for now:

- terminal `xgdb` is the most reliable path
- DDD should be useful for viewing C source and driving common commands
- more advanced DDD features may not work until `xgdb` grows a fuller
  compatibility layer

## Run It From VS Code

This directory now contains ready-to-use VS Code files:

- [tests/debug/.vscode/tasks.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/tasks.json)
- [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)

Open `tests/debug` itself as the VS Code workspace folder.

There are now two VS Code workflows:

1. task-based terminal workflow
2. native F5 workflow through the `xgdb-vsix` DAP extension

### Native DAP Workflow

The repo now contains a dedicated VS Code extension project in:

- [tools/xgdb-vsix/package.json](/home/tstih/data/retro-vault/xyz/tools/xgdb-vsix/package.json)
- [tools/xgdb-vsix/extension.js](/home/tstih/data/retro-vault/xyz/tools/xgdb-vsix/extension.js)
- [tools/xgdb-vsix/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tools/xgdb-vsix/.vscode/launch.json)

This extension contributes a new debugger type named `xgdb` and starts:

```text
xgdb --interpreter=dap --quiet
```

The included [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)
now also contains:

```json
{
  "name": "xgdb sieve (dap)",
  "type": "xgdb",
  "request": "launch",
  "program": "${workspaceFolder}/../../bin/z80/bin/apps/debug/sieve.bin",
  "cdb": "${workspaceFolder}/../../bin/z80/bin/apps/debug/sieve.cdb",
  "remoteTarget": "127.0.0.1:9000",
  "debuggerPath": "${workspaceFolder}/../../bin/bin/xgdb",
  "cwd": "${workspaceFolder}",
  "stopOnEntry": true
}
```

To test it:

1. Open `tools/xgdb-vsix` in VS Code.
2. Run the `Run xgdb VSIX on sieve` launch configuration.
3. Start your target separately so it is already listening on
   `127.0.0.1:9000`.
4. In the Extension Development Host window that opens, select
   `xgdb sieve (dap)` and press `F5`.
5. Open [sieve.c](/home/tstih/data/retro-vault/xyz/tests/debug/sieve.c),
   set breakpoints, then step and inspect state through the standard VS Code
   debug UI.

If you step into or over a linked library function that has symbols but
no source file:

- VS Code may not have a C source editor location to show for that stop
- `xgdb` now reports that honestly instead of pointing at a fake file
- the DAP frontend now supports disassembly fallback, so the session can
  continue in assembly instead of freezing on an attempted source step

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
      "label": "run xgdb-z80 sieve target",
      "type": "process",
      "command": "${workspaceFolder}/run_xgdb_z80.sh",
      "dependsOn": "build sieve debug sample",
      "isBackground": true,
      "problemMatcher": []
    },
    {
      "label": "stop xgdb-z80 sieve target",
      "type": "process",
      "command": "${workspaceFolder}/stop_xgdb_z80.sh",
      "problemMatcher": []
    }
  ]
}
```

### How To Use The Tasks

Inside VS Code:

1. Run `Tasks: Run Task`
2. Choose `run xgdb-z80 sieve target`
3. Open `tests/debug/sieve.c` in the editor

This is the target-only half of the workflow. Pair it with the DAP
launch configuration described above, or connect manually from another
terminal if you want to use raw `xgdb`.

The DAP launch configuration does not start or stop the target for you.
Use the provided tasks only if you want a manual convenience wrapper
around the reference `xgdb-z80` target.

### Included `launch.json`

The included `launch.json` is the native `xgdb` DAP configuration:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "xgdb sieve (dap)",
      "type": "xgdb",
      "request": "launch",
      "program": "${workspaceFolder}/../../bin/z80/bin/apps/debug/sieve.bin",
      "cdb": "${workspaceFolder}/../../bin/z80/bin/apps/debug/sieve.cdb",
      "remoteTarget": "127.0.0.1:9000",
      "debuggerPath": "${workspaceFolder}/../../bin/bin/xgdb",
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
4. Select `xgdb sieve (dap)`
5. Press `F5`

VS Code should then:

1. launch `xgdb` in DAP mode
2. connect to the already-running remote target
3. drive the session through the native VS Code debug UI

Stepping behavior worth knowing:

- `F10` / `next` now uses a temporary return breakpoint when it has
  stepped into a callee with no source, instead of trying to single-step
  the entire function one instruction at a time
- if no source exists for the current function, use the disassembly view
  or the terminal `disassemble` command to inspect progress

### What To Expect From The F5 Path

This is the preferred VS Code path now. The older `cppdbg` / MI sample
configuration has been removed from `tests/debug/.vscode` to avoid
accidentally selecting the wrong debugger backend.

### Best Current VS Code Layout

A practical layout is:

1. left editor pane: `tests/debug/sieve.c`
2. bottom terminal 1: `xgdb-z80`
3. bottom terminal 2: `xgdb`

Then when `xgdb` stops on `_sieve`, use:

```text
list
```

to print the active source context directly in the debugger terminal
while the same file is open in the editor.

### Future Direction

Once `xgdb` grows either:

- a fuller MI implementation, or
- a real UDAP adapter for VS Code

this README can be upgraded to a real `launch.json`-based F5 workflow.

## Useful Commands

Set a breakpoint at the C function:

```text
(xgdb) break _sieve
(xgdb) continue
```

Show current source:

```text
(xgdb) list
```

Show available functions:

```text
(xgdb) info functions
```

Show registers:

```text
(xgdb) info registers
```

Disassemble around the current PC:

```text
(xgdb) disassemble
```

Or from a specific symbol:

```text
(xgdb) x/12i _sieve
```

Inspect the sieve result globals in memory:

```text
(xgdb) x/4xb _prime_count
(xgdb) x/4xb _last_prime
(xgdb) x/80xb _flags
```

Single-step one instruction:

```text
(xgdb) stepi
```

Detach:

```text
(xgdb) detach
```

## What To Expect

When `sieve()` finishes:

- `_prime_count` should contain the number of primes from `2` to `64`
- `_last_prime` should contain the last prime in that range
- the `flags` array shows which values remained marked

Since the startup stub halts after `main` returns, continuing without a
breakpoint will eventually stop with a `halted` reason.
