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

`xemu` can read defaults from `xemu.conf`. It auto-searches:

- `./xemu.conf`
- `~/.config/x/xemu.conf`

The format is one `key = value` assignment per line, with `#` or `;`
comments. Command-line flags override config-file values.

Run as a remote target for `xgdb`:

```sh
bin/x/bin/xemu --listen 127.0.0.1:9000 --load-bin test.bin --origin 0x0100 --pc 0x0100
```

Run a sparse Intel HEX image directly:

```sh
bin/x/bin/xemu --run --load-ihx test.ihx --pc 0x0000
```

Run a raw program directly, mapping Z80 port `0` to host stdin and Z80
port `1` to host stdout:

```sh
bin/x/bin/xemu --run --load-bin test.bin --stdin-port 0 --stdout-port 1
```

Run a `--platform=emu` binary with the libc console ABI wired to host
stdin/stdout:

```sh
bin/x/bin/xemu --run --load-ihx test.ihx --pc 0x0000 --emu-stdio
```

Run a `--platform=emu` binary with both console I/O and host-backed file I/O:

```sh
bin/x/bin/xemu --run --load-ihx test.ihx --pc 0x0000 --emu-stdio --fs-root ./emu-fs
```

The default `platform=emu` console ports are:

- stdin status: `0xe2`
- stdin data: `0xe3`
- stdout: `0xe1`

The default `platform=emu` file ABI uses command port `0xe0` and request
mailboxes in high RAM. `--fs-root` binds those syscalls to a host directory so
`fopen`, `fread`, `fwrite`, `remove`, `rename`, and `lseek` can be exercised
from tests or direct emulator runs without touching the real project tree.

They can be overridden with `--stdin-status-port`, `--stdin-data-port`, and
`--stdout-port`.

## Banked Memory

`xemu` now supports a generic memory-map model:

- `store.<name>.*` defines backing storage and bank count
- `selector.<name>` defines a current bank selector
- `window.<name>.*` maps a CPU address range onto a store
- `port_rule.<name>.*` updates a selector from an `OUT` port write
- `port_rule.<name>.port_mask` optionally matches only selected port bits

This lets one port drive one banked window, several windows together, or
multiple independent selectors from different bitfields in the same port.

### Iskra Delta Partner-style layout

This matches a machine with shared `0x0000-0x7fff`, banked `0x8000-0xbfff`,
and shared `0xc000-0xffff`:

```ini
stdout_port = 0x01

store.low.size = 0x8000
store.bank.banks = 2
store.bank.size = 0x4000
store.high.size = 0x4000

selector.bank = 0

window.low.range = 0x0000-0x7fff
window.low.store = low

window.bank.range = 0x8000-0xbfff
window.bank.store = bank
window.bank.selector = bank

window.high.range = 0xc000-0xffff
window.high.store = high

port_rule.bank.port = 0x0010
port_rule.bank.selector = bank
```

### ZX Spectrum 128-style layout

This shows one port controlling two selectors at once: ROM from bit 4 and the
top `0xc000-0xffff` RAM bank from bits `0..2`.

```ini
store.rom.banks = 2
store.rom.size = 0x4000
store.rom.writable = false

store.mid.size = 0x8000

store.top.banks = 8
store.top.size = 0x4000

selector.rom = 0
selector.top = 0

window.rom.range = 0x0000-0x3fff
window.rom.store = rom
window.rom.selector = rom

window.mid.range = 0x4000-0xbfff
window.mid.store = mid

window.top.range = 0xc000-0xffff
window.top.store = top
window.top.selector = top

port_rule.rom.port = 0x00fd
port_rule.rom.port_mask = 0x00ff
port_rule.rom.selector = rom
port_rule.rom.mask = 0x10
port_rule.rom.shift = 4

port_rule.top.port = 0x00fd
port_rule.top.port_mask = 0x00ff
port_rule.top.selector = top
port_rule.top.mask = 0x07
```

### Compatibility Shortcut

The older four-page 16K model is still available for simple setups:

```sh
bin/x/bin/xemu --run --load-bin test.bin \
  --shared-pages 0,1,3 --banked-pages 2 \
  --bank-count 4 --bank-port 0x50
```

Equivalent `xemu.conf`:

```ini
shared_pages = 0,1,3
banked_pages = 2
bank_count = 4
bank_port = 0x50
stdout_port = 0x01
```
