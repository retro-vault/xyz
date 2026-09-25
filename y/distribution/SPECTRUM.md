# YOS Spectrum release

This directory is a shareable release of the universal 16 KiB YOS ROM.
`arch/48/`, `arch/128/`, and `arch/next/` contain the model-labelled binary payloads; their
contents are intentionally identical because the ROM detects the machine at
boot and the XPRG programs are hardware-independent. Keep the selected
directory.s `shell.sys` in the root of the esxDOS filesystem.

The three launchers use the bundled esxDOS 0.8.9 runtime, create private
media, and start ZEsarUX:

```sh
./run-48.sh
./run-128.sh
./run-next.sh
```

The runtime, firmware ROMs, and original notices are under
`firmware/esxdos089/`. `--esxdos DIR` or `ESXDOS` can override that copy.
Use `--prepare-only` to create media and print the exact emulator command
without opening a window, and `--work-dir DIR` to control where private media
is written. The launchers also accept `--headless` to require that the
packaged shell actually renders. They auto-detect ZEsarUX's installed
`128.rom` and `tbblue.mmc`; explicit `--rom128` and `--next-mmc` options
override them. Those stock machine images come from ZEsarUX and are not
duplicated in this package.

Host requirements are Python 3, ZEsarUX and hdfmonkey. The Next launcher also
uses mtools (`mcopy`). Its private MMC adds a `YOS` personality and keeps the
original image untouched. No separate esxDOS download or path is required.

Everything outside `arch/` is shared. `include/` contains
only `yos.h` and `yos.inc`; firmware, launch machinery, and sample source also
occur once. GPX types and calls are part of the single `yos_t` ABI. The minimal C and
assembly shell sources are under `samples/`; their
built `shell.sys` and `shell-asm.sys` images are in each model directory. Both
draw `Hello World!` at screen centre without loading a library or allocating
heap memory, then loop forever.
