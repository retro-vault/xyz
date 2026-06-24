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
| `--listen HOST:PORT` | Listen address in debugger-target mode |
| `--run` | Execute immediately instead of waiting for a debugger |
| `--max-steps N` | Step budget for `--run` mode |
| `--load-bin FILE` | Load a raw binary into memory |
| `--origin ADDR` | Binary load address |
| `--pc ADDR` | Initial program counter |
| `--sp ADDR` | Initial stack pointer |
| `--stdin-port ADDR` | Map Z80 port `ADDR` to host stdin |
| `--stdout-port ADDR` | Map Z80 port `ADDR` to host stdout |
| `-q`, `--quiet` | Quiet startup |

## Host Library

The same emulator core is available as `libxemu.a`, with staged headers in
`include/xemu/`.

That library provides:

- in-process machine control for test harnesses
- memory/register inspection and mutation
- breakpoint-aware run/step helpers
- an `rsp::target` adapter
- a small remote-session wrapper for talking to a running `xemu`
