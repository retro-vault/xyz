# CP/M 3 example

This directory contains examples only for the `cpm3` platform.

Build the Hello World transient program from the repository root:

```sh
mkdir -p build/examples/cpm3
bin/x/bin/xcc -Os --platform=cpm3 --oformat=binary \
  x/examples/cpm3/hello.c -o build/examples/cpm3/hello.com
```

The result is a CP/M `.COM` program loaded at `0x0100`.
