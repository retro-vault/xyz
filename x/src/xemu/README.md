# xemu

`xemu` is the standalone Z80 emulator in the X tools product.

It serves two related roles:

- a debugger target for `xgdb` over the GDB Remote Serial Protocol
- a headless execution engine for emulator-driven tests and console-style
  program runs

Built binary:

- `bin/x/bin/xemu`

Reusable library:

- `bin/x/lib/libxemu.a`

## Build

```sh
make -C x/src/xemu all
make -C x/src/xemu test
```

## Examples

Run as a remote target for `xgdb`:

```sh
bin/x/bin/xemu --listen 127.0.0.1:9000 --load-bin test.bin --origin 0x0100 --pc 0x0100
```

Run a raw program directly, mapping Z80 port `0` to host stdin and Z80
port `1` to host stdout:

```sh
bin/x/bin/xemu --run --load-bin test.bin --stdin-port 0 --stdout-port 1
```
