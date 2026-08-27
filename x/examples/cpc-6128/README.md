# Amstrad CPC 6128 example

This example exercises the full staged libc plus AMSDOS-backed file hooks on
the `cpc-6128` platform. Build its standard data disk from the repository root:

```sh
mkdir -p build/examples/cpc-6128
bin/x/bin/xcc -Os --platform=cpc-6128 --oformat=binary \
  x/examples/cpc-6128/files.c -o build/examples/cpc-6128/files.bin
bin/x/bin/xprog --dsk build/examples/cpc-6128/files.bin \
  -o build/examples/cpc-6128/files.dsk --name FILES.BIN
```

Insert the disk and type `RUN"FILES"`. The binary loads and enters at `0x4000`
and returns to BASIC when `main` finishes.
