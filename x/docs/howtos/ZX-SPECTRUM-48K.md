# ZX Spectrum 48K targets

X ships two freestanding ZX Spectrum 48K platforms:

- `zx-ram` builds a program for the standard Sinclair 48K ROM environment;
- `zx-rom` builds a complete 16 KiB replacement ROM.

All target startup, console, keyboard, heap, exit, and system-call hooks are
hand-written Z80 assembly. Both targets use the same libc and compiler runtime
as the other X platforms.

## Build the toolchain

From the repository root:

```sh
make -C x
```

This stages the target headers, CRTs, linker scripts, platform archives,
runtime, and libc under `bin/x/z80/`. No ZX-specific environment variable is
needed; `xcc` and `xld` locate these files relative to `bin/x/bin/`.

## Run the included text demo

The sample deliberately prints two Lorem Ipsum paragraphs so wrapping,
proportional spacing, and the 12-pixel line pitch are easy to inspect:

```sh
mkdir -p build/examples/zx-ram build/examples/zx-rom

bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary \
  x/examples/zx-ram/lorem.c -o build/examples/zx-ram/lorem.bin
bin/x/bin/xprog --tap build/examples/zx-ram/lorem.bin \
  -o build/examples/zx-ram/lorem.tap --name LOREM
bin/x/bin/xprog --tzx build/examples/zx-ram/lorem.bin \
  -o build/examples/zx-ram/lorem.tzx --name LOREM

bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary \
  x/examples/zx-rom/lorem.c -o build/examples/zx-rom/lorem.rom
```

The RAM binary is directly loadable at `0x5CCB`. At a normal 48K BASIC prompt,
load either tape image with:

```text
LOAD ""
```

With Fuse installed, the same artifacts can be run from the host:

```sh
fuse --machine 48 --auto-load --tape build/examples/zx-ram/lorem.tap
fuse --machine 48 --rom-48 build/examples/zx-rom/lorem.rom
```

The replacement-ROM form needs no Sinclair ROM file. The demo intentionally
keeps `main` alive after it finishes printing so an emulator can repaint the
completed framebuffer.

Some Snap-launched desktop shells inject GTK variables that conflict with a
non-Snap Fuse installation. If Fuse reports a GTK or `libpthread` symbol error,
launch it with those inherited variables removed:

```sh
env -u GTK_EXE_PREFIX -u GTK_IM_MODULE_FILE -u GTK_MODULES -u GTK_PATH \
  -u GDK_PIXBUF_MODULEDIR -u GDK_PIXBUF_MODULE_FILE \
  fuse --machine 48 --rom-48 build/examples/zx-rom/lorem.rom
```

## RAM program

For another C source, the minimal workflow is:

```sh
bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary main.c -o app.bin
bin/x/bin/xprog --tap app.bin -o app.tap --name APP
bin/x/bin/xprog --tzx app.bin -o app.tzx --name APP
```

The linked image begins at `0x5CCB`, the first byte after the documented 48K
ROM system variables and the lowest practical address for a normal ROM-loaded
program. Startup disables interrupts, selects a stack at `0xFFFF`, clears BSS,
copies initialized storage, initializes the console, and calls `main`. The
heap begins after the linked image and ends before `0xF000`; the upper 4 KiB is
reserved for stack growth.

The memory contract is:

| Address range | Use |
|---|---|
| `0x0000`–`0x3FFF` | Standard Sinclair 48K ROM |
| `0x4000`–`0x57FF` | Spectrum bitmap display |
| `0x5800`–`0x5AFF` | Display attributes |
| `0x5B00`–`0x5CCA` | ROM system variables and work area |
| `0x5CCB`–linked end | Program code, constants, and static storage |
| linked end–`0xEFFF` | libc heap |
| `0xF000`–`0xFFFF` | Reserved stack area, with initial SP at `0xFFFF` |

### Why the tape can load at `0x5CCB`

The TAP/TZX bootstrap is safe at this unusually low address. Its auto-start
BASIC program calls a 30-byte machine loader stored after a `REM` token. The
loader reads the following CODE header, arranges for the standard ROM tape
routine at `0x0556` to return directly to the selected entry, and lets the CODE
block overwrite the entire BASIC program.

`xprog` emits a program header/data pair followed by a CODE header/data pair.
Every Spectrum block carries the standard XOR checksum. TZX output is version
1.20 and contains four standard-speed data blocks with one-second pauses.

The tape name must contain 1–10 bytes. The entry must lie within the input
binary, and the binary must fit between the selected load address and
`0xFFFF`. The default load and entry address is `0x5CCB`; use
`--load-address` and `--entry` to override them together when packaging a
different fixed-address binary.

## Replacement ROM

```sh
bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary main.c -o app.rom
```

The result is always exactly 16,384 bytes for addresses `0x0000`–`0x3FFF`.
It includes reset, restart, interrupt, and NMI vectors, so Fuse or real
hardware can install it as the machine ROM without a separate loader.

The ROM target uses this layout:

| Address range | Use |
|---|---|
| `0x0000`–`0x3FFF` | Replacement ROM: vectors, code, constants, font, and packed initial data |
| `0x4000`–`0x57FF` | Spectrum bitmap display |
| `0x5800`–`0x5AFF` | Display attributes |
| `0x5B00`–linked end | Writable static state and BSS |
| linked end–`0xEFFF` | libc heap |
| `0xF000`–`0xFFFF` | Reserved stack area, with initial SP at `0xFFFF` |

Writable `_DATA` has a packed load image inside ROM. Startup copies that image
to RAM, applies the ordinary `_INITIALIZER`/`_INITIALIZED` contract, clears
BSS, initializes the console, and enters `main`. `xld` supplies the load-image
symbols `s__DATA_LOAD` and `l__DATA_LOAD`; it rejects any link whose resident
or copied bytes do not fit the 16 KiB output window.

The GNU linker script expresses the split as `>ram AT>rom`. The equivalent
SDCC-style script uses `AREA _DATA = 5B00` plus `COPY _DATA`. See the
[xld manual](../dist/man/XLD.md#rom-load-addresses) for the generic VMA/LMA
contract.

## Console and Tamsyn font

The assembly console borrows the Spectrum bitmap-row addressing and scrolling
structure from YOS. It does not call the Sinclair ROM, which makes the same
renderer usable in RAM programs and replacement ROMs.

The renderer provides:

- printable ASCII `0x20`–`0x7E`;
- proportional, pixel-granular horizontal placement;
- a 6-pixel maximum glyph width plus one pixel of spacing;
- 12-pixel glyph height and 16 visible text rows;
- carriage return, newline, edge wrapping, and 12-pixel scrolling;
- white ink on black paper with a black border after initialization.

The font is Tamzen 6x12r from the Tamsyn family. It was exported with
[snatch](https://github.com/retro-vault/snatch) from
[Font Vault's Tamsyn source](https://github.com/retro-vault/font-vault/tree/main/source/fonts/tamsyn)
as a proportional Partner bitmap stream. The checked-in stream contains an
eight-byte header, 96 little-endian glyph offsets, and one `bmp_t` record per
character; the renderer consumes that format directly.

Cursor advance must treat both an arithmetic carry and an exact wrap from
pixel 255 to pixel 0 as a newline. The Lorem demo exercises the dense wrapping
case that originally exposed this Z80 `INC`/carry edge condition.

## Keyboard and terminal I/O

Include `<conio.h>` and call `kbhit()` for the hardware-facing
non-blocking API. Each call scans the eight-row Spectrum keyboard matrix once
and returns the current ASCII key as a positive `int`, or zero when no
non-shift key is down. It is level-triggered: a held key is returned on each
poll. CAPS SHIFT selects uppercase/control mappings, SYMBOL SHIFT selects
punctuation, ENTER produces carriage return (`'\r'`), and the cursor keys map
through the usual CAPS SHIFT combinations.

`getchar` and `read(0, ...)` add blocking semantics by repeatedly calling that
same poller for a press and then for release. The matrix scanner is not
duplicated.

The descriptor behavior is intentionally small:

| Operation | ZX behavior |
|---|---|
| `kbhit()` | Non-blocking current ASCII key, or `0` |
| `read(0, ...)`, `getchar`, console `stdio` input | Blocking keyboard input |
| `write(1, ...)`, `write(2, ...)`, `putchar`, console `stdio` output | Tamsyn bitmap console |
| `close(0)`, `close(1)`, `close(2)` | Success |
| `read`/`write` on other descriptors | `-1` |
| `open`, `lseek`, `rename`, `remove`/`unlink`, file-backed `stdio` | Unsupported; failure result |
| `gettimeofday`, `settimeofday` | Unsupported; `-1` |
| `time`, `clock` | Standard failure value derived from the missing clock |
| `timespec_get` | `0`, because no requested time base is available |

The rest of libc—including allocation, strings, conversions, sorting,
searching, formatted console I/O, math, and the compiler runtime—uses the
normal staged assembly library. There is no hidden C platform support library.

`exit` stores the status in the public `zx_exit_status` word, disables
interrupts, and enters a permanent HALT loop. A normal program therefore does
not return to BASIC. A visual demo may instead remain in its own live loop,
as the included Lorem example does.

## Automated hardware-level regression

After building X, run the optional end-to-end test with
[zx-spectrum-mcp](https://github.com/retro-vault/zx-spectrum-mcp) and a legal
16 KiB 48K ROM image:

```sh
python3 x/tests/tests/zx48/run_mcp.py \
  --mcp /path/to/zx-spectrum-mcp \
  --rom /path/to/48.rom
```

The script builds and executes all four delivery forms:

1. raw RAM binary loaded at `0x5CCB`;
2. replacement ROM;
3. TAP loaded by typing `LOAD ""` into the real ROM;
4. TZX played as a standard-speed waveform through the real ROM.

Every case validates initialized and zero-filled storage, relocated pointers,
heap allocation, core libc string/search/sort/conversion functions, deliberate
file/time failures, console scrolling, idle and held-key `kbhit()` calls,
normal/CAPS/SYMBOL blocking keyboard input, exit, and a final `0xA5` memory
marker. A successful run ends with:

```text
PASS RAM binary (...=0xA5)
PASS replacement ROM (...=0xA5)
PASS TAP (...=0xA5)
PASS TZX (...=0xA5)
```

## Source map

| Path | Purpose |
|---|---|
| `x/platforms/zx-ram/` | Self-contained RAM CRT, scripts, hooks, console, keyboard, and font assembly |
| `x/platforms/zx-rom/` | Self-contained ROM CRT, scripts, hooks, console, keyboard, and font assembly |
| `x/src/xprog/` | TAP/TZX packager implementation and tests |
| `x/examples/zx-ram/` | RAM, TAP, and TZX Lorem Ipsum example |
| `x/examples/zx-rom/` | Replacement-ROM Lorem Ipsum example |
| `x/tests/tests/zx48/` | C stdlib smoke program and MCP runner |

## Deliberate limitations

- This is a 48K target; there is no 128K paging or bank-aware CRT.
- The console is monochrome and keeps one uniform attribute map.
- `kbhit()` is level-triggered and single-key oriented; it is not an
  event queue.
- There is no generic filesystem, tape API, wall clock, or return-to-BASIC
  protocol in the minimal backend.
- TAP/TZX packaging accepts a fixed-address flat binary, not an XL image.

Platform directories are one-to-one with selectable targets. There is no
`zx-common` pseudo-target; the small amount of mirrored ZX assembly is kept in
both target directories so each platform remains structurally independent.
