# xemu — emulator

Standalone Z80 emulator for the X tools suite.

`xemu` can run in two modes:

- headless execution mode for automated tests and console-style programs
- GDB Remote Serial Protocol server mode for `xgdb` and other compatible
  clients

## Quick start

```bash
# Run as a remote target for xgdb
xemu --listen 127.0.0.1:9000 --load-bin app.bin --origin 0x0100 --pc 0x0100

# Run directly, mapping Z80 port 0 to stdin and Z80 port 1 to stdout
xemu --run --load-bin app.bin --stdin-port 0 --stdout-port 1
```

## Options

| Option | Meaning |
|---|---|
| `--config FILE` | Load defaults from FILE, with command-line flags overriding config values |
| `--listen HOST:PORT` | Listen address in debugger-target mode |
| `--run` | Execute immediately instead of waiting for a debugger |
| `--max-steps N` | Step budget for `--run` mode |
| `--load-bin FILE` | Load a raw binary into memory |
| `--load-ihx FILE` | Load Intel HEX records into memory |
| `--origin ADDR` | Binary load address |
| `--pc ADDR` | Initial program counter |
| `--sp ADDR` | Initial stack pointer |
| `--emu-stdio`, `--no-emu-stdio` | Enable or disable the `platform=emu` stdio ABI mapping |
| `--fs-root DIR` | Bind `platform=emu` file syscalls to a host directory |
| `--stdin-port ADDR` | Map Z80 port `ADDR` to host stdin |
| `--stdin-status-port ADDR` | Map a Z80 status port to host stdin readiness |
| `--stdin-data-port ADDR` | Map a Z80 data port to host stdin bytes |
| `--stdout-port ADDR` | Map Z80 port `ADDR` to host stdout |
| `--shared-pages LIST` | Compatibility banking shortcut for shared pages |
| `--banked-pages LIST` | Compatibility banking shortcut for banked pages |
| `--bank-count N` | Compatibility banking shortcut: number of banks |
| `--bank-port ADDR` | Compatibility banking shortcut: OUT port that selects the bank |
| `-q`, `--quiet` | Quiet startup |
| `--no-quiet` | Disable quiet mode even if enabled in config |
| `--version` | Print version and exit |
| `-h`, `--help` | Show usage and exit |

## Host Library

The same emulator core is available as `libxemu.a`, with staged headers in
`include/xemu/`.

That library provides:

- in-process machine control for test harnesses
- memory/register inspection and mutation
- breakpoint-aware run/step helpers
- an `rsp::target` adapter
- a small remote-session wrapper for talking to a running `xemu`
