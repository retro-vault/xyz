# ZX Spectrum esxDOS disk example

`diskio.c` belongs to the `zx-esxdos` platform. It creates or replaces
`XCCDISK.TXT` in the current disk directory, writes a message, flushes it,
seeks to the beginning, reads and verifies it, and closes the file. The file
remains on disk for inspection. The program prints `Disk round-trip: OK` on
success and halts when `main` returns.

Build from the repository root after `make -C x`:

```sh
mkdir -p build/examples/zx-esxdos
bin/x/bin/xcc -Os --platform=zx-esxdos --oformat=binary \
  x/examples/zx-esxdos/diskio.c -o build/examples/zx-esxdos/DISKIO.BIN
```

Copy `DISKIO.BIN` to a writable disk used by a normally booted 48K Spectrum
with divIDE and esxDOS 0.8.9. At the BASIC prompt:

```text
CLEAR 32767
LOAD *"DISKIO.BIN" CODE 32768
RANDOMIZE USR 32768
```

The `*` selects the current disk even when a tape image is attached. The
binary has no tape or DOS header: it loads and enters at `0x8000`.

The example uses standard `<fcntl.h>` and `<unistd.h>` calls; XCC's `open`
takes exactly two arguments. No platform-private include is needed for
these basic file operations. See the [esxDOS guide](../../docs/dist/man/ZX-ESXDOS.md)
for the API, firmware requirements and memory map.

`lowram.c` demonstrates reclaiming RAM after the BASIC loader has finished.
It creates a separate heap over `0x5B00`–`0x7FFF` with the existing
`heap_init_arena` API, allocates 9,216 bytes, writes a pattern to
`LOWRAM.DAT`, closes and reopens the file, and checks every byte read back.
It frees the allocation and verifies that the arena can supply it again.
The screen and the program loaded at `0x8000` stay outside this arena.

```sh
bin/x/bin/xcc -Os --platform=zx-esxdos --oformat=binary \
  x/examples/zx-esxdos/lowram.c -o build/examples/zx-esxdos/LOWRAM.BIN
```

Copy `LOWRAM.BIN` to the disk and enter:

```text
CLEAR 32767
LOAD *"LOWRAM.BIN" CODE 32768
RANDOMIZE USR 32768
```

Reclaiming this region discards BASIC's workspace after program entry;
the example takes ownership of the machine and halts instead of returning
to BASIC. It prints `Lower RAM disk round-trip: OK (9216 bytes)` and leaves
`LOWRAM.DAT` on disk. Ordinary `malloc` continues to use the separate upper
arena. The example has passed the actual Sinclair BASIC disk loader and
esxDOS 0.8.9 on emulated divIDE with both `-Os`/`-Of` and both caller ABIs.
