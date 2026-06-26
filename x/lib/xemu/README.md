# libxemu

`libxemu` is the host-side Z80 emulator library used by the standalone
`xemu` tool and by emulator-driven test code.

It provides:

- an in-process Z80 machine built on `xz80`
- flat 64 KiB memory load/read/write helpers
- breakpoint-aware run and single-step control
- optional stdin/stdout port binding for console-style programs
- split stdin status/data port binding for the `platform=emu` console ABI
- a convenience `bind_emu_stdio()` helper for the default `platform=emu`
  console ports (`0xe2`/`0xe3` in, `0xe1` out)
- an `rsp::target` adapter so the same machine can be exposed to `xgdb`
- a small remote-session wrapper for talking to a running `xemu`

The build produces a static archive at `bin/x/lib/libxemu.a`.

Public headers live in [x/lib/xemu/include/xemu](/home/tstih/data/retro-vault/xyz/x/lib/xemu/include/xemu):

- [xemu.h](/home/tstih/data/retro-vault/xyz/x/lib/xemu/include/xemu/xemu.h)

Typical use:

```cpp
#include <xemu/xemu.h>
```

## Build

```sh
make -C x/lib/xemu all
make -C x/lib/xemu test
```
