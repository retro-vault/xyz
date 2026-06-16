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

- `bin/z/z80/bin/apps/debug/sieve.bin`
- `bin/z/z80/bin/apps/debug/sieve.cdb`

The build path is:

1. `xcc -g -S` compiles `sieve.c` to `sieve.s` and emits `sieve.adb`
2. `xas --mode=sdcc` assembles `sieve.s` to `sieve.rel`
3. `xas --mode=sdcc` assembles `crt0.s` to `crt0.rel`
4. `xas --mode=sdcc` assembles the runtime helpers from
   `src/xc/xcc/lib/runtime/`, recursively across the grouped subdirectories
   such as `int8/`, `int16/`, `int32/`, `int64/`, `float/`, `atomic/`,
   `jumps/`, `common/`, and `sys/`
5. `xar rcs` packs those helpers into `runtime.lib`
6. `xld -g` links the sample objects against `runtime.lib` into `sieve.bin` and `sieve.cdb`

## Run The Reference Emulator

In one terminal:

```sh
bin/x/bin/xgdb-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/z/z80/bin/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

This loads the binary at `0x0100`, starts execution at `_entry`, and
provides a remote debug target on TCP port `9000`.

## Run The Debugger

In a second terminal:

```sh
bin/x/bin/xgdb \
    --exec bin/z/z80/bin/apps/debug/sieve.bin \
    --cdb bin/z/z80/bin/apps/debug/sieve.cdb \
    --remote 127.0.0.1:9000 \
    -d tests/debug
```

## Run It From DDD

You can also point GNU DDD at `xgdb` as a custom debugger command.

Start `xgdb-z80` first:

```sh
bin/x/bin/xgdb-z80 \
    --listen 127.0.0.1:9000 \
    --load-bin bin/z/z80/bin/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

Then, from the repo root, launch DDD like this:

```sh
ddd --debugger "bin/x/bin/xgdb --quiet --exec bin/z/z80/bin/apps/debug/sieve.bin --cdb bin/z/z80/bin/apps/debug/sieve.cdb --remote 127.0.0.1:9000 -d tests/debug"
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

- it does expose a CLI and MI entry point
- it does speak the remote target protocol through `librsp`
- DDD integration depends on the subset of command-line behavior DDD
  expects from the inferior debugger

So for now:

- terminal `xgdb` is the most reliable path
- DDD should be useful for viewing C source and driving common commands
- more advanced DDD features may not work until `xgdb` grows a fuller
  compatibility layer

## Run It From VS Code

This directory contains helpful VS Code workspace files:

- [tests/debug/.vscode/tasks.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/tasks.json)
- [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)

Open `tests/debug` itself as the VS Code workspace folder.

What works today:

- the task-based workflow for building the sample and starting/stopping
  `xgdb-z80`
- editing `sieve.c` while driving `xgdb` manually from a terminal

What is still in progress:

- the `xgdb-vsix` extension project in `tools/xgdb-vsix/`
- the DAP-oriented launch configuration in
  [tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/tests/debug/.vscode/launch.json)

The current extension scaffolding and launch file are not yet a supported
end-to-end path, because:

- the extension launcher still tries `xgdb --interpreter=dap --quiet`
- the live `xgdb` command-line entry point currently exposes only `cli`
  and `mi`
- the extension manifest still models an older `symbols` / `.xgdb` input
  shape rather than the current `.cdb` plus optional `.map` flow

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

This is the supported VS Code workflow today. After the target is running,
connect from another terminal with:

```sh
bin/x/bin/xgdb \
    --exec bin/z/z80/bin/apps/debug/sieve.bin \
    --cdb bin/z/z80/bin/apps/debug/sieve.cdb \
    --remote 127.0.0.1:9000 \
    -d tests/debug
```

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

Once the `xgdb-vsix` configuration and the live `xgdb` entry point agree on
the debugger protocol and symbol-loading shape, this README can be expanded
again into a real F5 workflow.

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
