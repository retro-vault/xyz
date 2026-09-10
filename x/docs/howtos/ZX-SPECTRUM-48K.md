# ZX Spectrum 48K targets

X ships four ZX Spectrum 48K platforms:

- `zx-ram` builds a program for the standard Sinclair 48K ROM environment;
- `zx-esxdos` adds divIDE/divMMC disk I/O to a RAM program booted with esxDOS;
- `zx-rom` builds a complete 16 KiB replacement ROM with console services;
- `zx-esxdos-rom` executes an esxDOS disk application from a 16 KiB base ROM.

All target startup, console, keyboard, heap, exit, and system-call hooks are
hand-written Z80 assembly. All four targets use the same libc and compiler
runtime as the other X platforms.

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

## Disk programs with divIDE and esxDOS

Use `zx-esxdos` for a Spectrum booted with esxDOS 0.8.9 and a mounted FAT
disk. This separate target loads and enters at `0x8000`, keeping clear of
the active BASIC loader. After entry, its own stack, disabled interrupts
and halt-on-exit contract let an application reuse `0x5B00`–`0x7FFF`.
Supported external file calls keep internal buffers in divIDE RAM; they do
not reserve this Spectrum range. The bitmap console and keyboard are the
same as the other targets. The default heap ends at `_STACK`, `0xF000`.

```sh
bin/x/bin/xcc -Os --platform=zx-esxdos --oformat=binary \
  x/examples/zx-esxdos/diskio.c -o DISKIO.BIN
```

Copy `DISKIO.BIN` to the disk and run from BASIC:

```basic
CLEAR 32767
LOAD *"DISKIO.BIN" CODE 32768
RANDOMIZE USR 32768
```

The separate [low-RAM example](../../examples/zx-esxdos/lowram.c) retains
that loading procedure and then initializes a private heap arena over
`0x5B00`–`0x7FFF`. It uses a 9,216-byte allocation for a verified disk
round trip. Reclaim this memory only after entering the application;
the program cannot subsequently return to BASIC or use its ROM services.

The example creates `XCCDISK.TXT`, writes and flushes its contents, seeks
back, verifies a read, and closes it. The target implements `open`, `close`,
`read`, `write`, `lseek`, `fsync`, `unlink`, `rename`, `mkdir`, `rmdir`,
`chdir`, `getcwd`, `stat`, and `fstat`. File-backed `stdio`, including
`fopen`/`fread`/`fwrite`/`fseek`/`fclose`, uses these hooks. As elsewhere in
this libc, `open` takes exactly two arguments; `creat` accepts a mode for
source compatibility. FAT permission arguments are ignored.

The adapter translates XCC descriptors and error codes, preserves IX/IY,
implements append before every write, and converts POSIX seek origins to
esxDOS's different seek modes. It uses the firmware's actual position query
after a seek. Directory status normalizes the firmware's sentinel size to
zero. Termination closes active disk descriptors before halting.

The [installed esxDOS guide](../dist/man/ZX-ESXDOS.md) describes the supported
flags, path/buffer limits, metadata and firmware restrictions. In particular,
esxDOS can clamp seeks to EOF and its rename does not replace an existing
destination atomically. The plain `zx-rom` target retains its console-only
hooks. The separate `zx-esxdos-rom` target supplies the base-ROM entry points
used by esxDOS, boots the firmware and calls it through small RAM gates
while the application executes from ROM. Its workflow is described below.

The implementation was checked against the fresh z88dk external-call wrappers
and the [official esxDOS 0.8.9 distribution](https://www.esxdos.org/). It uses
the actual 0.8.9 services, including directory operations whose older z88dk
configuration comments still describe them as unimplemented.

## Boot a disk application from ROM

`zx-esxdos-rom` builds a complete 16 KiB base ROM that boots with divIDE
esxDOS 0.8.9 and runs the application directly from ROM. It does not require
or bundle the Sinclair ROM. The divIDE keeps its separate firmware, and its
disk must contain matching `SYS` files with `AutoBoot=0` in
`SYS/CONFIG/ESXDOS.CFG`.

```sh
mkdir -p build/examples/zx-esxdos-rom
bin/x/bin/xcc -Os --platform=zx-esxdos-rom --oformat=binary \
  x/examples/zx-esxdos-rom/diskio.c \
  -o build/examples/zx-esxdos-rom/DISKIO.ROM
```

Use `DISKIO.ROM` as the Spectrum's base ROM. The disk example starts from
reset without a BASIC loader and exercises the same file API as the RAM
target. Code and constants remain in ROM. Startup copies only writable
native `_DATA` and ELF `.data`, including 48 bytes of fixed disk-call gates,
clears both BSS forms and enters `main` in ROM. Each RAM gate executes
`RST 08`; firmware pages itself out before returning to the gate, which
then returns to the calling ROM instruction.

Pathnames and write input can reside in ROM. Each ROM pathname uses a
temporary stack copy of its actual length plus NUL, up to 256 bytes.
`rename` copies only its ROM arguments, up to 512 bytes for two maximum-length
paths. File writes from ROM use a 128-byte stack buffer in
chunks. RAM inputs pass directly to firmware, and read/metadata destinations
remain writable RAM.

| Address range | Use |
|---|---|
| `0x0000`–`0x3FFF` | Executing code, constants, bootstrap and initial data; firmware maps here temporarily |
| `0x4000`–`0x5AFF` | Display bitmap and attributes |
| `0x5B00`–linked end | Writable data, 48-byte disk-call gates and BSS |
| linked end–`0xEFFF` | libc heap |
| `0xF000`–`0xFFFF` | Default descending stack allowance |

The ROM target reclaims 9,472 bytes by starting writable storage at `0x5B00`
instead of `0x8000`. Stock esxDOS 0.8.9's supported external file calls need
no permanent Spectrum workspace: filesystem buffers live in divIDE RAM.
Firmware uses some Spectrum RAM during boot; the CRT reuses it only after
boot returns. Caller buffers, ordinary call frames and ROM-input stack
copies still occupy application RAM.

Both disk targets default to a 4 KiB stack allowance, not a disk-I/O minimum.
The heap limit comes from `_STACK` in the linker script. Copy the script,
pass it with `-T`, and move `_STACK` and its `RESERVE` together; for GNU
scripts, also change the RAM region length so it ends at the new boundary.
For example, `F800` permits a 2 KiB stack allowance with initial SP still
`FFFF`. See the installed disk guides for exact script settings.

Both linker scripts reserve divIDE's instruction-fetch traps at `0x04C6`,
`0x0562` and `0x3D00`–`0x3DFF`, with jump guards before each hole. The ROM
must hold these reservations plus startup, code, constants and initial data.
Original divIDE has no software bit that disables automatic paging; the RAM
gates use its normal entry/return protocol to map firmware for each call.
BASIC, the NMI browser, dot commands and 128K banking are outside this
target's contract. Program return closes disk descriptors and halts. See the
[installed ROM guide](../dist/man/ZX-ESXDOS-ROM.md), including the explicit
GNU-mode build recipe, and the
[ROM disk example](../../examples/zx-esxdos-rom/README.md).

The disk example uses 100 bytes of static RAM including the gates, excluding
stack and heap, in all three profiles. The preceding layout passed 43 native
S/M/L and GNU M configurations, with all 43 recorded-tool images
byte-identical to those executions; twelve ROM ABI lanes and all four
original Spectrum MCP modes also passed. All 18,047 compiler regression
variants passed with their recorded tools; that linker separately passed
103 normal/sanitized component tests and a four-mode Spectrum MCP run.
See the
[direct ROM execution record](../../tests/tests/zx48/esxdos/xip-validation-2026-09.json).
The final reclaimed-RAM layout passes 67 stock-firmware cold boots with
FAT16/FAT32 and 18 deterministic ABI lanes. Four low-arena RAM example
images match successful BASIC-loader boots. The final linker passes 156
normal/sanitized tests plus executable reserved-hole and REL/ELF addend
checks; all 48 latest XCC benchmark cells pass without size or cycle changes.
All four original Spectrum MCP modes pass with the final staged tools.
See the separate
[compact-RAM record](../../tests/tests/zx48/esxdos/compact-ram-validation-2026-09.json)
for these results and their final tool identities.
The [earlier record](../../tests/tests/zx48/esxdos/rom-validation-2026-09.json)
remains historical evidence for the whole-application RAM-copy design.

## Replacement ROM

```sh
bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary main.c -o app.rom
```

The plain `zx-rom` result is always exactly 16,384 bytes for addresses
`0x0000`–`0x3FFF`.
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

Include `<stdio.h>` and call `trygetchar()` for the hardware-facing
non-blocking API. Each call scans the eight-row Spectrum keyboard matrix once
and returns the current ASCII key as a positive `int`, or zero when no
non-shift key is down. It is level-triggered: a held key is returned on each
poll. CAPS SHIFT selects uppercase/control mappings, SYMBOL SHIFT selects
punctuation, ENTER produces carriage return (`'\r'`), and the cursor keys map
through the usual CAPS SHIFT combinations.

`getchar` and `read(0, ...)` add blocking semantics by repeatedly calling that
same poller for a press and then for release. The matrix scanner is not
duplicated.

The plain `zx-ram` and `zx-rom` descriptor behavior is intentionally small;
the `zx-esxdos` and `zx-esxdos-rom` file operations are described above:

| Operation | ZX behavior |
|---|---|
| `trygetchar()` | Non-blocking current ASCII key, or `0` |
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
interrupts, and enters a permanent HALT loop. The esxDOS targets also attempt
to close all disk descriptors first. A normal program therefore does
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
file/time failures, console scrolling, idle and held-key `trygetchar()` calls,
normal/CAPS/SYMBOL blocking keyboard input, exit, and a final `0xA5` memory
marker. A successful run ends with:

```text
PASS RAM binary (...=0xA5)
PASS replacement ROM (...=0xA5)
PASS TAP (...=0xA5)
PASS TZX (...=0xA5)
```

The esxDOS targets have separate deterministic ABI and real-firmware disk
tests. `run_rom_firmware.py` performs a pristine cold boot with the custom
base ROM and real esxDOS firmware. See the
[disk test instructions](../../tests/tests/zx48/esxdos/README.md) and
[current compact-RAM validation record](../../tests/tests/zx48/esxdos/compact-ram-validation-2026-09.json).
The [earlier ROM record](../../tests/tests/zx48/esxdos/rom-validation-2026-09.json)
retains 27 configurations of the superseded RAM-copy implementation:
11 executions with its recorded final tools and 16 other successful
executions with complete ROM byte identity after rebuilding with those
same tools. The four plain MCP delivery modes above were also rerun
successfully with that recorded build.

## Source map

| Path | Purpose |
|---|---|
| `x/platforms/zx-ram/` | Self-contained RAM CRT, scripts, hooks, console, keyboard, and font assembly |
| `x/platforms/zx-esxdos/` | RAM target with esxDOS disk hooks and post-entry low-RAM reuse |
| `x/platforms/zx-rom/` | Self-contained ROM CRT, scripts, hooks, console, keyboard, and font assembly |
| `x/platforms/zx-esxdos-rom/` | esxDOS boot ROM, direct ROM execution, RAM call gates and disk hooks |
| `x/src/xprog/` | TAP/TZX packager implementation and tests |
| `x/examples/zx-ram/` | RAM, TAP, and TZX Lorem Ipsum example |
| `x/examples/zx-esxdos/` | Disk create/write/flush/seek/read example |
| `x/examples/zx-rom/` | Replacement-ROM Lorem Ipsum example |
| `x/examples/zx-esxdos-rom/` | Disk example that starts from reset in a custom base ROM |
| `x/tests/tests/zx48/` | C stdlib smoke program and MCP runner |
| `x/tests/tests/zx48/esxdos/` | Disk ABI probes, real-firmware RAM tests and pristine ROM cold-boot tests |

## Deliberate limitations

- This is a 48K target; there is no 128K paging or bank-aware CRT.
- The console is monochrome and keeps one uniform attribute map.
- `trygetchar()` is level-triggered and single-key oriented; it is not an
  event queue.
- The plain `zx-ram` and `zx-rom` targets have no filesystem. `zx-esxdos`
  requires initialized esxDOS; `zx-esxdos-rom` boots matching esxDOS 0.8.9
  with `AutoBoot=0`. Both retain the documented filesystem limits.
- `zx-esxdos-rom` executes its application from 16 KiB ROM, using small RAM
  gates for firmware calls. It provides no BASIC, NMI browser or dot commands.
- There is no tape API, wall clock, or return-to-BASIC protocol in these
  backends.
- TAP/TZX packaging accepts a fixed-address flat binary, not an XL image.

Platform directories are one-to-one with selectable targets. There is no
`zx-common` pseudo-target; the small amount of mirrored ZX assembly is kept in
each target directory so every platform remains structurally independent.
