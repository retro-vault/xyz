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
| `--origin <addr>` | Raw/XL download origin |
| `--pc <addr>` | PC to set after download |
| `--no-load` | Do not download `--exec` after connecting |
| `-d <dir>`, `--directory <dir>` | Add source search directory |
| `--interpreter cli\|mi\|mi2\|dap`, `--mi` | Front-end mode (CLI default; MI and DAP for IDEs) |
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

When `--remote` and `--exec` are both set, `xgdb` downloads the program to the
RSP target before installing breakpoints. ELF files are loaded by section VMA
and default PC to the ELF entry. Intel HEX records use their encoded
addresses. XL files are relocated at `--origin` and default PC to
`origin + entry_point`; when the selected program is XL, CDB/ELF/MAP symbol
addresses are biased by the same origin. Raw binaries are written at
`--origin`.
