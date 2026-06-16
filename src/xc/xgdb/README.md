# xgdb

This directory contains the current debugger front-end and the current
reference remote target.

Built binaries:

- `bin/x/bin/xgdb`
- `bin/x/bin/xgdb-z80`

## Components

`xgdb`

- a reasonably minimal GDB-like command-line debugger
- loads SDCC `.cdb` debug information and `.map` linker output for source-level debugging
- talks to remote targets through `librsp`
- falls back cleanly to symbol/disassembly-only debugging when a linked
  function has no source file

`xgdb-z80`

- a small reference Z80 remote target
- uses the vendored `z80ex` CPU core
- loads a raw binary into 64K memory and exposes it through the stub
  protocol

`xgdb-z80` is not meant to replace your full emulator. Its real job is
to give the debugger stack a known-good target for bring-up and tests.

## Build

```sh
make -C src/xc/xgdb all
```

## xgdb Usage

Basic form:

```sh
bin/x/bin/xgdb --exec yos.rom --cdb yos.cdb --map yos.map --remote 127.0.0.1:9000
```

Supported startup switches:

- `--exec <file>`
- `--cdb <file>`   SDCC CDB debug information
- `--map <file>`   SDCC MAP linker output (optional, supplements CDB)
- `--remote <host:port>`
- `-ex <command>`
- `-q`, `--quiet`
- `-h`, `--help`

If `--cdb` is not given and `--exec foo.bin` is set, `xgdb` also
tries `foo.bin.cdb` and `foo.bin.map` as sidecars.

For machine-interface (IDE) integration use:

- `--interpreter=mi` or `--mi`

The protocol abstraction layer also has a DAP-shaped direction in the code,
but the current `xgdb` entry point does not yet expose
`--interpreter=dap`.

For DDD integration use `xgdb` directly:

```sh
ddd --debugger bin/x/bin/xgdb
```

DDD classifies debuggers partly by executable name. Now that the real
binary is named `xgdb`, DDD selects its GDB frontend and passes the
expected `-q -fullname` startup flags.

The long-term goal for the interactive command language is GNU GDB
compatibility, not emulating the SDCC debugger command set.

## xgdb Commands

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
bin/x/bin/xgdb-z80 --listen 127.0.0.1:9000 --load-bin yos.rom --origin 0x100 --pc 0x100
```

Connect with the debugger:

```sh
bin/x/bin/xgdb --exec yos.rom --cdb yos.cdb --map yos.map --remote 127.0.0.1:9000
```

Then inside `xgdb`:

```text
(xgdb) status
(xgdb) info registers
(xgdb) break _main
(xgdb) continue
(xgdb) list
(xgdb) info locals
(xgdb) x/16xb 0x100
(xgdb) x/8i 0x100
(xgdb) stepi
(xgdb) detach
```

## Disassembly Behavior

Instruction display goes through `libxgdb`:

- decoder: Z80
- formatter: SDCC-style Z80 syntax

That means output is intentionally shaped like SDCC/ASxxxx-flavored Z80,
for example:

```text
ld	a, #0x42
ld	a, 5(ix)
ld	-2(ix), a
```

If a function exists in symbols but has no source file entry, `xgdb`
still knows its address range and can debug it through:

- `disassemble`
- `x/Ni ADDR`

The debugger will no longer pretend that such functions belong to a
previous source file just to keep source stepping alive.

## xgdb-z80 Usage

Basic form:

```sh
bin/x/bin/xgdb-z80 --listen 127.0.0.1:9000 --load-bin test.bin --origin 0x0000 --pc 0x0000 --sp 0xFFFF
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
- a GDB-style CLI and MI interface for IDE integration
- a GDB Remote Serial Protocol transport path through `librsp`

What it is not yet:

- a DAP mode exposed by the main `xgdb` executable
- a source-level expression evaluator
- a call stack unwinder
- a full emulator integration for Iskra Delta Partner devices

Those are natural next steps, but this layer is already enough to start
debugger-driven bring-up against a remote Z80 target.
