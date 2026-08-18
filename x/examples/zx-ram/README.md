# ZX Spectrum 48K RAM example

This directory contains examples only for the `zx-ram` platform.

[`lorem.c`](lorem.c) exercises the proportional Tamsyn console, wrapping, and
scrolling. Build the directly loadable binary plus both tape formats from the
repository root:

```sh
mkdir -p build/examples/zx-ram
bin/x/bin/xcc -Os --platform=zx-ram --oformat=binary \
  x/examples/zx-ram/lorem.c -o build/examples/zx-ram/lorem.bin
bin/x/bin/xprog --tap build/examples/zx-ram/lorem.bin \
  -o build/examples/zx-ram/lorem.tap --name LOREM
bin/x/bin/xprog --tzx build/examples/zx-ram/lorem.bin \
  -o build/examples/zx-ram/lorem.tzx --name LOREM
```

Run the TAP image in Fuse:

```sh
fuse --machine 48 --auto-load --tape build/examples/zx-ram/lorem.tap
```

The binary and tape payloads load and enter at `0x5CCB`.
