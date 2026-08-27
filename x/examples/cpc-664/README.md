# Amstrad CPC 664 example

This example uses the `cpc-664` firmware and AMSDOS-backed file hooks. Build a
standard CPC data disk containing the program from the repository root:

```sh
mkdir -p build/examples/cpc-664
bin/x/bin/xcc -Os --platform=cpc-664 --oformat=binary \
  x/examples/cpc-664/files.c -o build/examples/cpc-664/files.bin
bin/x/bin/xprog --dsk build/examples/cpc-664/files.bin \
  -o build/examples/cpc-664/files.dsk --name FILES.BIN
```

Insert the disk and type `RUN"FILES"`. The example creates and reads
`RESULT.TXT` through the standard `open`, `read`, `write`, and `close` calls.
