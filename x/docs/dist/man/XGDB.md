# xgdb — debugger

Source-level debugger for programs built with `xcc -g`. Comes in two
parts:

- **xgdb** — the debugger front end. Speaks the familiar GDB command
  language (CLI) or GDB/MI for IDE integration.
- **xgdb-z80** — a Z80 gdbserver with a built-in emulator. Speaks the
  GDB Remote Serial Protocol over TCP, so any GDB-compatible client can
  connect to it too.

## Quick start

```bash
# 1. Build with debug info
xcc -g main.c -o app.xl

# 2. Start the target (Z80 emulator + gdbserver)
xgdb-z80 --listen 127.0.0.1:9000 &

# 3. Connect the debugger
xgdb --exec app.xl --cdb app.cdb --remote 127.0.0.1:9000
```

Then use the usual GDB commands: `break main`, `run`, `next`, `step`,
`print x`, `backtrace`, `continue`, `quit`.

## xgdb options

| Option | Meaning |
|---|---|
| `--exec <file>` | Target binary image |
| `--cdb <file>` | Debug information file (produced by `xld -g`) |
| `--map <file>` | Linker map file (optional) |
| `--remote <host:port>` | Connect to a remote target |
| `-d <dir>` | Add source search directory |
| `--interpreter cli\|mi`, `--mi` | Front-end mode (CLI default; MI for IDEs) |
| `-ex <command>` | Execute a debugger command on startup |
| `--log <file>` | Log all protocol I/O |

## xgdb-z80 options

| Option | Meaning |
|---|---|
| `--listen HOST:PORT` | Listen address (default `127.0.0.1:9000`) |
| `--load-bin FILE` | Load a raw binary into memory |
| `--origin ADDR` | Binary load address (default `0x0000`) |
| `--pc ADDR` | Initial program counter (default: origin) |
| `--sp ADDR` | Initial stack pointer (default `0xFFFF`) |
| `-q`, `--quiet` | Quiet startup |
