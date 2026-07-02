# xgdb — debugger

Source-level debugger for programs built with `xcc -g`.

`xgdb` is the debugger front end. It speaks a familiar GDB-style CLI and
GDB/MI for IDE integration. Pair it with `xemu` or any other compatible
RSP target.

## Quick start

```bash
# 1. Build with debug info
xcc -g main.c -o app.xl

# 2. Start the target (Z80 emulator + gdbserver)
xemu --listen 127.0.0.1:9000 &

# 3. Connect the debugger
xgdb --exec app.xl --cdb app.cdb --remote 127.0.0.1:9000
```

Then use the usual debugger commands: `break main`, `run`, `stepi`,
`info registers`, `continue`, `quit`.

## xgdb options

| Option | Meaning |
|---|---|
| `--exec <file>` | Target binary image |
| `--cdb <file>` | Debug information file (produced by `xld -g`) |
| `--map <file>` | Linker map file (optional) |
| `--remote <host:port>` | Connect to a remote target |
| `-d <dir>`, `--directory <dir>` | Add source search directory |
| `--interpreter cli\|mi\|mi2`, `--mi` | Front-end mode (CLI default; MI for IDEs) |
| `-ex <command>` | Execute a debugger command on startup |
| `--log <file>` | Log all protocol I/O |
| `--version` | Print version and exit |
| `-h`, `--help` | Show usage and exit |

## Compatibility flags

These are accepted for GNU GDB / DDD front-end compatibility and are
currently ignored:

- `-q`, `--quiet`
- `--nx`, `-nx`
- `--fullname`, `-fullname`
- `--tty <path>`
- `--tty=<path>`

For target-side emulator options, see `XEMU.md`.
