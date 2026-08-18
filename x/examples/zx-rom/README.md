# ZX Spectrum 48K replacement-ROM example

This directory contains examples only for the `zx-rom` platform.

[`lorem.c`](lorem.c) exercises the proportional Tamsyn console, wrapping, and
scrolling from a complete replacement ROM. Build it from the repository root:

```sh
mkdir -p build/examples/zx-rom
bin/x/bin/xcc -Os --platform=zx-rom --oformat=binary \
  x/examples/zx-rom/lorem.c -o build/examples/zx-rom/lorem.rom
```

Run the exact 16 KiB image in Fuse:

```sh
fuse --machine 48 --rom-48 build/examples/zx-rom/lorem.rom
```

The program deliberately remains alive after printing so the completed
framebuffer remains visible.
