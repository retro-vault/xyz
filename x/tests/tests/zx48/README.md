# ZX Spectrum 48K target smoke test

`smoke.c` exercises initialized and zero-filled storage, relocated pointers,
heap allocation, strings, sorting/searching, conversion, console output and
scrolling, non-blocking `<stdio.h>` `trygetchar()`, blocking input derived from it,
and the deliberately unsupported clock/filesystem hooks.

Build both target forms with the staged toolchain:

```sh
bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary smoke.c -o smoke.bin
bin/x/bin/xprog --tap smoke.bin -o smoke.tap
bin/x/bin/xprog --tzx smoke.bin -o smoke.tzx
bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary smoke.c -o smoke.rom
```

Success writes `0xA5` to `zx_smoke_result`, prints `ZX48 STDLIB PASS` with
the proportional Tamsyn console, returns zero, and halts through `_exit`.
The smoke program covers initialized and zero-filled storage, relocated
pointers, allocation/reallocation, strings, sorting/searching, conversion,
console output and scrolling, an empty and held-key poll, normal/CAPS/SYMBOL
blocking keyboard input, descriptor rules, and the deliberate file/time
failure surface.

`run_mcp.py` performs the complete optional hardware test: raw RAM binary,
replacement ROM, TAP, and TZX. It uses the standard 48K ROM for the RAM/tape
cases, types `LOAD ""`, plays the real tape waveform, supplies `q` to the
non-blocking poller, and then supplies the mixed normal/caps/symbol/ENTER
sequence requested by the blocking keyboard test:

```sh
python3 run_mcp.py --mcp /path/to/zx-spectrum-mcp --rom /path/to/48.rom
```

Run that command from this directory, or use the repository-root form:

```sh
python3 x/tests/tests/zx48/run_mcp.py \
  --mcp /path/to/zx-spectrum-mcp \
  --rom /path/to/48.rom
```

Expected final output is four passes:

```text
PASS RAM binary (...=0xA5)
PASS replacement ROM (...=0xA5)
PASS TAP (...=0xA5)
PASS TZX (...=0xA5)
```

The repository also contains separate visual long-text/Fuse examples for
[`zx-ram`](../../../examples/zx-ram/README.md) and
[`zx-rom`](../../../examples/zx-rom/README.md).
