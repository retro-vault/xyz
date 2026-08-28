# Quick Handoff for Returning Sessions

This is the short, high-signal state of the X toolchain product. Repository
working conventions and canonical commands remain in the root `AGENTS.md`.

## Latest completed milestone

XCC's current `-Of` campaign was driven by a source audit of the latest 80cc,
then implemented as generic typed-IR, CFG, liveness, alias, and register-
clobber rules. Both XCC M profiles pass the current-upstream 24-program suite.
Against the better valid 80cc frame-pointer or stack-pointer result per row,
`-Of` is smaller on 23/24 and faster on 24/24; `-Os` is smaller on 24/24 and
faster on 11/24. The same specialized profiles are respectively strict speed
and size winners on 24/24 against the broader valid SDCC/80cc envelope. The
compiler contains no benchmark-name, source-fragment,
magic-constant, or program-fingerprint recognition.

The reported automatic-structure member-store failure is fixed. The store was
correct; an unsafe framed sibling call released the frame before a terminal
observer dereferenced the local. Automatic-address materialization now blocks
that transform, and the exact direct/alias forms are permanent all-profile
regressions. Crossing bitfields exposed separate metadata bugs in address
folding and store availability; partial stores are no longer rewritten or
forwarded as complete words. The exact `item->arg = arg` spelling now runs at
all six optimization levels alongside the new optimizer and physical-register
interference cases.

The final bare sweep also caught byte-wrap-unsound affine pointer walking for
`(unsigned char)(i - 1)`. Pointer strength reduction now needs checked range
proofs for every byte intermediate, using only scoped canonical-loop guard,
preheader, and latch facts. The RLE reproducer passes every profile in S/M/L,
while proven non-wrapping nested row/column walks keep their fast form.

The fixed-project tiny-regex holdout found a second, independent pointer-walk
guard: a committed secondary cursor must use the secondary induction's own
unique initialization and update, not the sentinel induction's conditional
updates. That parser-shaped case now passes all six profiles in S/M/L and is a
permanent regression.

X now ships `cpc-464`, `cpc-664`, and `cpc-6128` firmware targets at `0x4000`.
The 464 is cassette-only and does not link AMSDOS state; the 664/6128 provide
ROM-backed raw and stdio disk operations, input seeking, rename, and remove.
`xprog --cdt` creates CPC firmware cassette records and `xprog --dsk` creates
standard CPCEMU/AMSDOS data disks. The three-model real-ROM MCP release validation
passes generated-media boot, libc, console, clock, writable file, and
error-path coverage. It is invoked explicitly and is not in the manifest-driven
regression pack.

Root `make packages` now builds and verifies both `bin/x/pkg/deb/x_*.deb` and
the XGDB VSIX. The Debian audit extracts the finished archive and checks every
packaged platform's CRT source/object, both linker formats, platform archive,
manuals, normalized ownership/modes, and CPC CDT/DSK switches. The extracted
package itself passes all three CPC MCP runs.

XCC integrates directly with z88dk's classic runtime through
`--runtime=z88dk-classic`. It derives printf/scanf handler capabilities from
literal formats, merges them across translation units through zcc's per-link
option file, and uses a conservative fallback for dynamic or escaped
formatters. The historical locked suite remains available unchanged; new
headline reports use 80cc as the primary competitor and keep SDCC visible as
a correctness lane and in the broader valid-competitor envelope.

X now ships complete minimal ZX Spectrum 48K RAM and replacement-ROM
platforms. `zx-ram` begins at `0x5CCB`; `zx-rom` emits an exact 16 KiB image.
Both use assembly-only CRT and platform hooks, the ordinary staged libc and
runtime, a YOS-derived proportional Tamsyn bitmap console, a public
non-blocking `<stdio.h>` `trygetchar()` API, and blocking libc input derived from
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
3. `x/docs/howtos/AMSTRAD-CPC.md` — CPC target, CDT/DSK, ROM I/O, and MCP
   contract.
4. `x/docs/CURRENT-STATUS.md` — detailed recent toolchain and libc state.
5. `x/docs/ARCHITECTURE.md` — product layout and staged-platform decisions.
6. `x/CHANGELOG.md` — canonical X release notes.

## Fast verification

```sh
make -C x
make packages
make -C x/pkg/debian check
bash x/tests/run_tests.sh --filter xcc
python3 x/tests/tests/zx48/run_mcp.py \
  --mcp /path/to/zx-spectrum-mcp \
  --rom /path/to/48.rom
python3 x/tests/tests/cpc/run_mcp.py \
  --mcp /path/to/amstrad-cpc-mcp \
  --roms /path/to/roms
```

The unified XCC L-model suite last passed 4,466/4,466. The ZX MCP run last passed all
four delivery modes and marker checks.

## Durable implementation locations

- `x/platforms/zx-ram/` and `x/platforms/zx-rom/` — self-contained target CRT,
  linker scripts, syscall hooks, console, keyboard, and font assembly.
- `x/platforms/cpc-464/`, `cpc-664/`, and `cpc-6128/` — self-contained CPC
  CRT, linker, firmware, and optional AMSDOS hooks.
- `x/src/xprog/` — XPRG plus TAP/TZX/CDT/DSK packaging.
- `x/pkg/` — Debian/VSIX orchestration and finished-archive verification.
- `x/src/xld/` and `x/lib/xbfd/` — placement and ROM load-image support.
- `x/libc/src/` — hand-written assembly libc.
- `x/tests/tests/c23/` — canonical manifest-driven compiler suite.

Update this handoff, `CURRENT-STATUS.md`, architecture notes, and the product
changelog whenever another platform or major linker/runtime contract lands.
