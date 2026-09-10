# ZX Spectrum esxDOS ROM disk example

This self-contained `zx-esxdos-rom` example builds a 16 KiB replacement ROM.
On reset it boots esxDOS and runs `diskio.c` directly from ROM. A 48-byte RAM
gate table handles firmware calls; ROM strings use temporary stack copies
for disk operations. The example creates or replaces `XCCDISK.TXT`, writes
and flushes a message, seeks back, verifies the contents and closes the file. Success
prints `Disk round-trip: OK`, leaves the file on disk and halts.

Build from the repository root after `make -C x`:

```sh
mkdir -p build/examples/zx-esxdos-rom
bin/x/bin/xcc -Os --platform=zx-esxdos-rom --oformat=binary \
  x/examples/zx-esxdos-rom/diskio.c \
  -o build/examples/zx-esxdos-rom/DISKIO.ROM
```

Use `DISKIO.ROM` as the Spectrum's base ROM. The divIDE must separately have
esxDOS 0.8.9 firmware and a writable disk with matching `SYS` files. Configure
`AutoBoot=0` in `SYS/CONFIG/ESXDOS.CFG`. The program starts from reset without
a Sinclair ROM or BASIC loader.

Writable storage starts at `0x5B00`, reclaiming 9,472 bytes from the earlier
layout. The example uses 100 static RAM bytes, including its 48-byte gates;
the application stack and heap are additional. The supported file calls
keep their internal buffers in divIDE RAM. The default 4 KiB stack allowance
can be changed in a copied linker script as described in the ROM guide.

For a ZEsarUX installation with its normal ROM/resource files available:

```sh
zesarux --noconfigfile --machine 48k \
  --romfile build/examples/zx-esxdos-rom/DISKIO.ROM \
  --ide-file /path/to/disk.ide --enable-ide --enable-divide \
  --divide-rom /path/to/ESXIDE.BIN
```

The example deliberately creates its own output file on that disk. Use a
private image when evaluating it in an emulator.

See the [ROM guide](../../docs/dist/man/ZX-ESXDOS-ROM.md) for the memory map,
firmware setup and supported operations.
