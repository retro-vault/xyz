# xdbg

This directory contains the first debugger front-end and the first
reference remote target.

Built binaries:

- `bin/bin/xc/xdbg/xdbg`
- `bin/bin/xc/xdbg/xdbg-z80`

## Components

`xdbg`

- a reasonably minimal GDB-like command-line debugger
- loads linked `.xdbg` symbol files through `libxdbg`
- talks to remote targets through `libxdbgstub`
- falls back cleanly to symbol/disassembly-only debugging when a linked
  function has no source file

`xdbg-z80`

- a small reference Z80 remote target
- uses the vendored `z80ex` CPU core
- loads a raw binary into 64K memory and exposes it through the stub
  protocol

`xdbg-z80` is not meant to replace your full emulator. Its real job is
to give the debugger stack a known-good target for bring-up and tests.

## Build

```sh
make -C src/xc/xdbg all
```

## xdbg Usage

Basic form:

```sh
bin/bin/xc/xdbg/xdbg --exec yos.rom --symbols yos.xdbg --remote 127.0.0.1:9000
```

Supported startup switches:

- `--exec <file>`
- `--symbols <file>`
- `--remote <host:port>`
- `-ex <command>`
- `-q`, `--quiet`
- `-h`, `--help`

If `--symbols` is not given and `--exec foo.bin` is set, `xdbg` also
tries `foo.bin.xdbg`.

For VS Code integration, `xdbg` also supports:

- `--interpreter=dap`

That mode now advertises and serves DAP disassembly requests, so editor
clients can show assembly when no source file exists for the current
function.

## xdbg Commands

Current interactive commands are:

- `help`
- `quit`
- `q`
- `target remote HOST:PORT`
- `symbol-file FILE`
- `file FILE`
- `break EXPR`
- `b EXPR`
- `delete [ID]`
- `continue`
- `c`
- `run`
- `stepi`
- `si`
- `nexti`
- `ni`
- `info registers`
- `info breakpoints`
- `info functions`
- `info files`
- `info locals`
- `list [SYMBOL|FILE:LINE]`
- `disassemble [EXPR] [COUNT]`
- `disas [EXPR] [COUNT]`
- `x/Nxb ADDR`
- `x/Ni ADDR`
- `status`
- `detach`
- `show version`
- `set pagination off`
- `pwd`

Address expressions currently supported by `break`, `disassemble`, and
`x`:

- numeric values such as `0x100`
- explicit address form like `*0x100`
- symbol names
- function names

## Example Session

Start the reference target:

```sh
bin/bin/xc/xdbg/xdbg-z80 --listen 127.0.0.1:9000 --load-bin yos.rom --origin 0x100 --pc 0x100
```

Connect with the debugger:

```sh
bin/bin/xc/xdbg/xdbg --exec yos.rom --symbols yos.xdbg --remote 127.0.0.1:9000
```

Then inside `xdbg`:

```text
(xdbg) status
(xdbg) info registers
(xdbg) break _main
(xdbg) continue
(xdbg) list
(xdbg) info locals
(xdbg) x/16xb 0x100
(xdbg) x/8i 0x100
(xdbg) stepi
(xdbg) detach
```

## Disassembly Behavior

Instruction display goes through `libxdbg`:

- decoder: Z80
- formatter: SDCC-style Z80 syntax

That means output is intentionally shaped like SDCC/ASxxxx-flavored Z80,
for example:

```text
ld	a, #0x42
ld	a, 5(ix)
ld	-2(ix), a
```

If a function exists in symbols but has no source file entry, `xdbg`
still knows its address range and can debug it through:

- `disassemble`
- `x/Ni ADDR`
- DAP disassembly requests from VS Code

The debugger will no longer pretend that such functions belong to a
previous source file just to keep source stepping alive.

## xdbg-z80 Usage

Basic form:

```sh
bin/bin/xc/xdbg/xdbg-z80 --listen 127.0.0.1:9000 --load-bin test.bin --origin 0x0000 --pc 0x0000 --sp 0xFFFF
```

Supported switches:

- `--listen HOST:PORT`
- `--load-bin FILE`
- `--origin ADDR`
- `--pc ADDR`
- `--sp ADDR`
- `-q`, `--quiet`
- `-h`, `--help`

Behavior today:

- 64K flat RAM
- raw binary load at `origin`
- Z80 register access through `z80ex`
- software-side breakpoint list checked before instruction execution
- `continue`, `step_instruction`, `pause`, `read/write memory`, `read/write registers`

When embedding the same transport in a real emulator, the intended
shutdown pattern is:

- run `serve()` inside a loop like `while (server.is_listening())`
- call `server.close()` from another thread when the emulator is
  shutting down

That now wakes a blocking `serve()` call even if it is waiting in
`accept()` or on an already connected client.

## What This Is And Is Not

What it is:

- enough debugger surface to inspect memory, symbols, lines, and locals
- enough remote target surface to test the debugger end to end
- a base for VS Code or UDAP integration

What it is not yet:

- GDB/MI
- GDB remote serial protocol
- a source-level expression evaluator
- a call stack unwinder
- a full emulator integration for Iskra Delta Partner devices

Those are natural next steps, but this layer is already enough to start
debugger-driven bring-up against a remote Z80 target.
