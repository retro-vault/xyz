# YOS process example

This is the smallest useful YOS process: it obtains the single `yos_t`
interface, draws `Hello World!` in the centre of the screen without a library
or heap allocation, and remains in an endless loop.

```sh
mkdir -p build/examples/yos bin/y/arch/48
bin/x/bin/xcc -Os --platform=yos x/examples/yos/hello.c \
  -o build/examples/yos/hello.xl
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 6 \
  build/examples/yos/hello.xl -o bin/y/arch/48/hello.prc
bin/x/bin/xprog --esxdos bin/y/arch/48/hello.prc \
  --name HELLO.PRC -o build/examples/yos/hello.ide
```

The final command creates a partitioned 16 MiB FAT16 IDE image with the
process in its root directory. To make it the boot shell instead, place the
same XPRG process on the disk as `shell.sys`.
