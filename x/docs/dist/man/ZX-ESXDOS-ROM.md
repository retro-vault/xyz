# ZX Spectrum esxDOS replacement-ROM platform

`--platform=zx-esxdos-rom` builds a standalone 16 KiB Spectrum ROM with disk
functions supplied by divIDE and esxDOS 0.8.9. It boots from reset without a
Sinclair ROM or BASIC loader. Application code and constants stay in ROM.
Only 48 bytes of disk-call gates execute in RAM: a gate pages in esxDOS for
one operation, then returns to ROM after firmware pages itself out.

## Firmware setup and build

Install matching esxDOS 0.8.9 firmware in the divIDE and matching `SYS` files
on its disk. Set `AutoBoot=0` in `SYS/CONFIG/ESXDOS.CFG`. The 16 KiB image built
below replaces the machine's base ROM; the divIDE firmware remains separate.

```sh
xcc -Os --platform=zx-esxdos-rom --oformat=binary main.c -o APP.ROM
```

`APP.ROM` is exactly 16,384 bytes. Install it as the base ROM or select it as
the base-ROM image in an emulator configured for a 48K Spectrum with divIDE.
It starts automatically when the machine resets. The generated image does
not require or bundle the Sinclair ROM; esxDOS firmware remains installed
separately in the divIDE.

The default command uses native SDCC-style objects and the compact linker
script. For actual GNU/ELF object output, supply the installed startup,
script and libraries explicitly; the GNU driver path has different default
library/startup selection:

```sh
X_PREFIX=/opt/x
"$X_PREFIX/bin/xcc" -Os --mode=gnu --platform=zx-esxdos-rom \
  --oformat=binary -nostartfiles \
  "$X_PREFIX/z80/lib/crt0-zx-esxdos-rom.rel" \
  -T "$X_PREFIX/z80/lib/linker-zx-esxdos-rom.ld" main.c \
  -L"$X_PREFIX/z80/lib" -lzx-esxdos-rom -lc -lruntime -o APP.ROM
```

Use the actual installation prefix, such as the repository's `bin/x`, for
`X_PREFIX`. Both routes produce a fixed 16 KiB ROM and use the same runtime
and filesystem implementation.

The repository example `x/examples/zx-esxdos-rom/diskio.c` creates or replaces
`XCCDISK.TXT`, writes and flushes a message, seeks to the beginning, reads and
verifies it, and closes the file. It prints `Disk round-trip: OK` on success.

## File API

The standard descriptor and stdio interfaces are the same as the
[esxDOS RAM target](ZX-ESXDOS.md): `open`, `close`, `read`, `write`, `lseek`,
`fsync`, `stat`, `fstat`, `unlink`, `rename`, `chdir`, `getcwd`, `mkdir`, and
`rmdir`, plus the normal file-backed libc operations. `open` takes exactly
two arguments. Descriptors 0–2 retain keyboard/screen behavior; up to 16 disk
slots are available, subject to firmware resources.

File offsets are signed 32-bit values and individual transfers are limited
to 32,767 bytes. Pathnames may reside in ROM or RAM and contain 1–255 bytes
before their terminating NUL. `write` accepts either ROM or RAM input;
`read`, `stat`, `fstat` and `getcwd` require writable RAM destinations.

While esxDOS is mapped, it cannot read the application's ROM. Each ROM
pathname therefore uses a temporary stack copy of `strlen(path)+1` bytes,
up to 256 bytes including NUL. `rename` copies only its ROM arguments;
the two copies total at most 512 bytes. `XCCDISK.TXT` needs a 12-byte copy.
File writes from ROM use a 128-byte stack buffer repeatedly until the
requested transfer completes.
RAM inputs pass directly to firmware. Console writes read ROM directly.
These copies add no permanent scratch buffers.

Rename does not replace an existing destination. Native transfer errors
return `-1` because firmware's partial byte count is unreliable, including
an error after earlier ROM-write chunks succeeded. Consult the
[RAM guide](ZX-ESXDOS.md) for the remaining filesystem limits and errors.

The ROM target is self-contained, with its own assembly hooks, console,
keyboard scanner, Tamsyn font and `sys/esxdos.h` header. It has no source
imports from another platform.

## Startup and memory

| Address range | Use |
|---|---|
| `0x0000`–`0x3FFF` | Executing application, constants, startup and initial data; divIDE temporarily maps its firmware here |
| `0x4000`–`0x5AFF` | Bitmap display and attributes |
| `0x5B00`–linked end | Writable data, 48-byte disk-call gates and zero-initialized storage |
| linked end–`0xEFFF` | libc heap |
| `0xF000`–`0xFFFF` | Default descending stack allowance |

The supported external file API in stock esxDOS 0.8.9 needs no permanent
Spectrum workspace below the application. Its filesystem buffers and
internal state occupy the divIDE's own RAM. Firmware boot temporarily uses
Spectrum RAM, but startup can reclaim it after firmware returns. Writable
storage now starts immediately after the display at `0x5B00`, recovering
9,472 bytes compared with the former `0x8000` start. Application buffers,
ordinary call frames and the temporary ROM-input copies remain separate
costs. The target does not return to BASIC or use its ROM services.

The header preserves the reset and restart entry instructions needed by
esxDOS's delayed ROM mapping, its character-output return and inline-call
byte reader, and an interrupt return that balances the saved AF register.
After esxDOS finishes booting, startup copies native `_DATA` and ELF `.data`
to RAM, including the 48-byte gate table, then initializes writable storage.
Code, constants and both initialization routines execute from ROM. The GNU
script uses `AT>rom` for writable data; its compact counterpart uses `COPY`.
The CRT clears native and ELF BSS, selects SP `0xFFFF` and IY `0x5C3A`,
initializes the bitmap console and calls `main` in ROM.

Original divIDE hardware automatically pages in its firmware on instruction
fetches at fixed addresses. Both scripts reserve `0x04C6`, `0x0562` and
`0x3D00`–`0x3DFF`; the linker keeps entire section contributions clear and
inserts jumps before these holes. Reset, interrupt and syscall vectors have
the instructions required by firmware. Port `0xE3` can force mapping, but
does not provide a software switch to disable these automatic entry points.
The gates use the normal firmware mapping and return protocol. See the
[hardware author's programming model](https://baze.sk/3sc/divide/files/pgm_model.txt).

The linker checks that the bootstrap, code, constants, initial data and
reserved holes fit in 16 KiB. No application code or constant image occupies
RAM. Program return records the exit status, attempts to close disk
descriptors 3–18 and halts.

For the supplied disk example, static RAM use is 100 bytes, including the
48-byte gates, in `-O0`, `-Os` and `-Of`. The earlier RAM-copy implementation
used 6,285, 5,330 and 5,415 bytes respectively. These figures exclude the
descending stack, temporary input copies and heap allocations; programs
with additional writable globals need additional RAM.

Both esxDOS targets default to a 4 KiB stack allowance at the top of RAM.
This is a shared toolchain default, not a minimum required by disk I/O.
The heap ends at the linker-defined `_STACK` boundary. To tune it, copy the
installed linker script and pass the copy with `-T`. In the compact script,
change both `AREA _STACK = F000` and `RESERVE F000-FFFF`. In the GNU script,
change `_STACK 0xF000`, the matching `RESERVE`, and the `ram` region length.
For a 2 KiB allowance use `F800`/`0xF800`, reserve through `FFFF`, and give
the ROM target's GNU `ram` region length `0x9D00`. Initial SP remains
`0xFFFF`. Choose space for the application's deepest calls, libc and disk
wrappers; the ROM pathname/write copies share this same stack.

This target is for the stated 48K/esxDOS setup. It does not provide BASIC,
esxDOS dot-command execution, the firmware NMI browser, 128K banking, or a
wall-clock service. Keep `AutoBoot=0`; automatic BASIC/snapshot startup uses
facilities supplied by a full Sinclair-compatible ROM.

The preceding direct-ROM layout passed 43 cold-boot configurations spanning
native S/M/L and GNU M, both caller ABIs, filesystem and ROM-input probes,
examples and a 10 KiB constant/initialization fixture. All 43 recorded-tool
images match the successful firmware executions byte for byte. Twelve
deterministic ROM ABI lanes passed 553 checks and 891 hostile firmware calls
each, and the four original Spectrum MCP modes passed with the compiler and
linker recorded for that layout. All 18,047 compiler regression variants
passed with their recorded tools; that linker separately passed 103
normal/sanitized component tests. The
repository's `xip-validation-2026-09.json` retains that validation and its
tool identities.

The reclaimed-RAM layout passes 67 stock-firmware cold-boot configurations
with FAT16/FAT32 and 18 deterministic ABI lanes using the final tools.
Four RAM low-arena example images also match successful BASIC-loader boots.
The final linker passes 156 tests in normal and sanitized builds, plus
executable reserved-hole and REL/ELF addend checks. All 48 latest XCC
benchmark cells pass with no size or cycle changes. All four original
Spectrum MCP modes pass with the final staged tools. These results and
their input identities are recorded separately in the repository's
`compact-ram-validation-2026-09.json`.

The earlier `rom-validation-2026-09.json` remains historical evidence for the
superseded implementation that relocated the complete application into RAM.
