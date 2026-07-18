# libxemu

`libxemu` is the host-side Z80 emulator library used by the standalone
`xemu` tool and by emulator-driven test code.

It provides:

- an in-process Z80 machine built on `xz80`
- flat or selector/window-based bank-switched memory mapping helpers
- breakpoint-aware run and single-step control
- optional stdin/stdout port binding for console-style programs
- split stdin status/data port binding for the `platform=emu` console ABI
- a convenience `bind_emu_stdio()` helper for the default `platform=emu`
  console ports (`0xe2`/`0xe3` in, `0xe1` out)
- loaders for raw binary, Intel HEX, and linked ELF images
- optional compatibility helpers for the older 4x16K page-switching model
- an `rsp::target` adapter so the same machine can be exposed to `xgdb`
- a small remote-session wrapper for talking to a running `xemu`

The build produces a static archive at `bin/x/lib/libxemu.a`.

Public headers live in [x/lib/xemu/include/xemu](/home/tstih/data/retro-vault/xyz/x/lib/xemu/include/xemu):

- [xemu.h](/home/tstih/data/retro-vault/xyz/x/lib/xemu/include/xemu/xemu.h)

Typical use:

```cpp
#include <xemu/xemu.h>
```

The generic mapper is built from:

- `memory_store_config` for backing storage and bank counts
- `memory_selector_config` for active bank selectors
- `memory_window_config` for CPU address ranges
- `memory_port_rule_config` for port-driven selector updates, with optional
  masked port matching

Call `machine::configure_memory_map()` to install a custom map. The older
`configure_banked_memory()` and `bind_bank_port()` helpers remain available as
compatibility shorthands for the fixed four-page model.

## Build

```sh
make -C x/lib/xemu all
make -C x/lib/xemu test
```
