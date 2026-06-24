# Debug Sample

This directory contains a tiny standalone Z80 C program meant for
debugging with the native X toolchain:

- `xcc`
- `xas`
- `xld`
- `xgdb`
- `xemu`

The sample is intentionally simple:

- no headers
- no libc
- no YOS services
- one C function, `sieve()`

It builds into:

- a flat binary linked by `xld`
- a linked `.cdb` sidecar emitted by `xld`

## Files

[sieve.c](/home/tstih/data/retro-vault/xyz/x/tests/debug/sieve.c)

- the sample C program

[crt0.s](/home/tstih/data/retro-vault/xyz/x/tests/debug/crt0.s)

- tiny startup stub
- calls `_main`
- halts forever after `main` returns

## Build

```sh
make -C x/tests/debug all
```

That produces:

- `bin/z/z80/bin/apps/debug/sieve.bin`
- `bin/z/z80/bin/apps/debug/sieve.cdb`

## Run The Reference Emulator

In one terminal:

```sh
bin/x/bin/xemu \
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
    -d x/tests/debug
```

## Run It From DDD

You can also point GNU DDD at `xgdb` as a custom debugger command.

Start `xemu` first:

```sh
bin/x/bin/xemu \
    --listen 127.0.0.1:9000 \
    --load-bin bin/z/z80/bin/apps/debug/sieve.bin \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
```

Then, from the repo root, launch DDD like this:

```sh
ddd --debugger "bin/x/bin/xgdb --quiet --exec bin/z/z80/bin/apps/debug/sieve.bin --cdb bin/z/z80/bin/apps/debug/sieve.cdb --remote 127.0.0.1:9000 -d x/tests/debug"
```

Useful commands:

- `break _sieve`
- `continue`
- `stepi`
- `info registers`
- `disassemble`
- `x/80xb _flags`

## Run It From VS Code

This directory contains helpful VS Code workspace files:

- [x/tests/debug/.vscode/tasks.json](/home/tstih/data/retro-vault/xyz/x/tests/debug/.vscode/tasks.json)
- [x/tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/x/tests/debug/.vscode/launch.json)

Open `x/tests/debug` itself as the VS Code workspace folder.

What works today:

- the task-based workflow for building the sample and starting/stopping
  `xemu`
- editing `sieve.c` while driving `xgdb` manually from a terminal

What is still in progress:

- the `xgdb-vsix` extension project in `x/pkg/xgdb-vsix/`
- the DAP-oriented launch configuration in
  [x/tests/debug/.vscode/launch.json](/home/tstih/data/retro-vault/xyz/x/tests/debug/.vscode/launch.json)

### Included `tasks.json`

The included `tasks.json` provides:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build sieve debug sample",
      "type": "shell",
      "command": "make -C x/tests/debug all",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "run xemu sieve target",
      "type": "process",
      "command": "${workspaceFolder}/run_xemu.sh",
      "dependsOn": "build sieve debug sample",
      "isBackground": true,
      "problemMatcher": []
    },
    {
      "label": "stop xemu sieve target",
      "type": "process",
      "command": "${workspaceFolder}/stop_xemu.sh",
      "problemMatcher": []
    }
  ]
}
```

### How To Use The Tasks

Inside VS Code:

1. Run `Tasks: Run Task`
2. Choose `run xemu sieve target`
3. Open `x/tests/debug/sieve.c` in the editor

Then connect from another terminal with:

```sh
bin/x/bin/xgdb \
    --exec bin/z/z80/bin/apps/debug/sieve.bin \
    --cdb bin/z/z80/bin/apps/debug/sieve.cdb \
    --remote 127.0.0.1:9000 \
    -d x/tests/debug
```

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

Show registers:

```text
(xgdb) info registers
```

Inspect the sieve result globals in memory:

```text
(xgdb) x/4xb _prime_count
(xgdb) x/4xb _last_prime
(xgdb) x/80xb _flags
```

Since the startup stub halts after `main` returns, continuing without a
breakpoint will eventually stop with a `halted` reason.
