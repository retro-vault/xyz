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
