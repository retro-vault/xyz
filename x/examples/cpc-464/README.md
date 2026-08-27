# Amstrad CPC 464 example

This directory contains examples only for the cassette-only `cpc-464`
platform. Build the firmware-hosted program and its standard CDT image from
the repository root:

```sh
mkdir -p build/examples/cpc-464
bin/x/bin/xcc -Os --platform=cpc-464 --oformat=binary \
  x/examples/cpc-464/hello.c -o build/examples/cpc-464/hello.bin
bin/x/bin/xprog --cdt build/examples/cpc-464/hello.bin \
  -o build/examples/cpc-464/hello.cdt --name HELLO
```

Insert the tape, type `RUN"!HELLO"`, and start playback. The firmware loads
and enters the binary at `0x4000`; returning from `main` returns to BASIC.
