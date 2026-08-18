# Quick Handoff for Returning Sessions

This is the short, high-signal state of the X toolchain product. Repository
working conventions and canonical commands remain in the root `AGENTS.md`.

## Latest completed milestone

X now ships complete minimal ZX Spectrum 48K RAM and replacement-ROM
platforms. `zx-ram` begins at `0x5CCB`; `zx-rom` emits an exact 16 KiB image.
Both use assembly-only CRT and platform hooks, the ordinary staged libc and
runtime, a YOS-derived proportional Tamsyn bitmap console, a public
non-blocking `<conio.h>` `kbhit()` API, and blocking libc input derived from
that poller. Files and wall-clock services deliberately fail.

`xprog` creates auto-running TAP and standard-speed TZX 1.20 images. `xld`
supports ROM load/run splits through GNU `AT>region` and SDCC `COPY area`, with
generated `s__AREA_LOAD`/`l__AREA_LOAD` symbols and overflow rejection.

The optional `x/tests/tests/zx48/run_mcp.py` regression passes raw RAM,
replacement ROM, TAP, and TZX against a real 48K ROM through
zx-spectrum-mcp. The target-owned `x/examples/zx-ram/lorem.c` and
`x/examples/zx-rom/lorem.c` Fuse demos visually validate the completed
proportional framebuffer. The demo found and closed the exact-pixel-256
cursor-wrap edge case.

## Read first

1. `AGENTS.md` — repository conventions and current build/test commands.
2. `x/docs/howtos/ZX-SPECTRUM-48K.md` — complete ZX build, memory, I/O,
   console, Fuse, and MCP contract.
3. `x/docs/CURRENT-STATUS.md` — detailed recent toolchain and libc state.
4. `x/docs/ARCHITECTURE.md` — product layout and staged-platform decisions.
5. `x/CHANGELOG.md` — canonical X release notes.

## Fast verification

```sh
make -C x
bash x/tests/run_tests.sh --filter xcc
python3 x/tests/tests/zx48/run_mcp.py \
  --mcp /path/to/zx-spectrum-mcp \
  --rom /path/to/48.rom
```

The unified XCC suite last passed 4,375/4,375. The ZX MCP run last passed all
four delivery modes and marker checks.

## Durable implementation locations

- `x/platforms/zx-ram/` and `x/platforms/zx-rom/` — self-contained target CRT,
  linker scripts, syscall hooks, console, keyboard, and font assembly.
- `x/src/xprog/` — XPRG plus TAP/TZX packaging.
- `x/src/xld/` and `x/lib/xbfd/` — placement and ROM load-image support.
- `x/libc/src/` — hand-written assembly libc.
- `x/tests/tests/c23/` — canonical manifest-driven compiler suite.

Update this handoff, `CURRENT-STATUS.md`, architecture notes, and the product
changelog whenever another platform or major linker/runtime contract lands.
