# xgdb

`xgdb` is the debugger frontend for programs built with `xcc -g`.

Built binary:

- `bin/x/bin/xgdb`

Related sibling tool:

- `bin/x/bin/xemu` — standalone Z80 emulator and remote debug target

## What xgdb does

- provides a GDB-like command-line debugger
- loads linked ELF debug/symbol information, SDCC `.cdb` debug information,
  and optional `.map` linker output
- talks to remote targets through `librsp`
- falls back cleanly to symbol/disassembly-only debugging when a linked
  function has no source file

## Build

```sh
make -C x/src/xgdb all
```

Run the MI smoke test:

```sh
make -C x/src/xgdb test
```

## xgdb Usage

Basic form:

```sh
bin/x/bin/xgdb --exec yos.rom --cdb yos.cdb --map yos.map --remote 127.0.0.1:9000
```

Supported startup switches:

- `--exec <file>` target image; ELF files also provide embedded symbols/debug
- `--cdb <file>`   SDCC CDB debug information
- `--map <file>`   SDCC MAP linker output (optional, supplements CDB)
- `--remote <host:port>`
- `-d <dir>`, `--directory <dir>`
- `--log <file>`
- `-ex <command>`
- `--version`
- `-h`, `--help`

If `--cdb` is not given and `--exec foo.elf` is set, `xgdb` first loads
embedded ELF symbols/debug information. For raw images such as `foo.bin`,
it tries `foo.cdb`, `foo.bin.cdb`, `foo.map`, and `foo.bin.map` sidecars.

For machine-interface (IDE) integration use:

- `--interpreter=mi` or `--mi`
- `--interpreter=mi2`

Example:

```sh
bin/x/bin/xgdb --interpreter=mi --exec yos.rom --cdb yos.cdb --map yos.map --remote 127.0.0.1:9000
```

The protocol abstraction layer also has a DAP-shaped direction in the code,
but the current `xgdb` entry point does not yet expose
`--interpreter=dap`.

Compatibility flags accepted and currently ignored:

- `-q`, `--quiet`
- `--nx`, `-nx`
- `--fullname`, `-fullname`
- `--tty <path>`
- `--tty=<path>`

For DDD integration use `xgdb` directly:

```sh
ddd --debugger bin/x/bin/xgdb
```

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
bin/x/bin/xemu --listen 127.0.0.1:9000 --load-bin yos.rom --origin 0x100 --pc 0x100
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

## Pairing With xemu

`xemu` is the default companion target for local debugger bring-up.

Common options:

- `--listen HOST:PORT`
- `--load-bin FILE`
- `--origin ADDR`
- `--pc ADDR`
- `--sp ADDR`
- `--run`
- `--stdin-port ADDR`
- `--stdout-port ADDR`

See [x/src/xemu/README.md](/home/tstih/data/retro-vault/xyz/x/src/xemu/README.md)
for the standalone emulator details and `libxemu` API.

## What This Is And Is Not

What it is:

- enough debugger surface to inspect memory, symbols, lines, and locals
- a GDB-style CLI and MI interface for IDE integration
- a GDB Remote Serial Protocol transport path through `librsp`

What it is not yet:

- a DAP mode exposed by the main `xgdb` executable
- a source-level expression evaluator
- a call stack unwinder

Those are natural next steps, but this layer is already enough to start
debugger-driven bring-up against a remote Z80 target.
