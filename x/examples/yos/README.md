# YOS process example

This sample uses only the `--platform=yos` libc backend and named YOS
services. It checks the cached kernel table, installs a process-local console
hook, allocates through the process-owned heap, round-trips a file through the
POSIX interface, and draws one pixel through the optional `gpx` service.

```sh
mkdir -p build/examples/yos bin/y/z80/spectrum/bin
bin/x/bin/xcc -Os --platform=yos x/examples/yos/hello.c \
  -o build/examples/yos/hello.xl
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 8 \
  build/examples/yos/hello.xl -o bin/y/z80/spectrum/bin/hello.sys
bin/x/bin/xprog --esxdos bin/y/z80/spectrum/bin/hello.sys \
  --name HELLO.SYS -o build/examples/yos/hello.ide
```

The final command creates a partitioned 16 MiB FAT16 IDE image with the
process in its root directory. To make it the boot shell instead, package the
process as `shell.sys` and place that name on the disk.
