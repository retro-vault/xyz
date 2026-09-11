# Your First Process

Build X and YOS once from the repository root:

```sh
make -C x
make -C y/src/z80
```

The compiler is `bin/x/bin/xcc`; the operating-system platform is selected by
`--platform=yos`. Start with `hello.c`:

```c
#include <yos.h>

int main(void)
{
    yos_t *yos = yos_get_api();
    return (!yos || yos->version() < YOS_VERSION) ? 1 : 0;
}
```

Compile and package it:

```sh
mkdir -p build/my-yos
bin/x/bin/xcc -Os --platform=yos hello.c -o build/my-yos/hello.xl
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 8 \
  build/my-yos/hello.xl -o build/my-yos/hello.sys
bin/x/bin/xprog --inspect build/my-yos/hello.sys
```

`--name` is the process name stored in XPRG metadata. `--stack-size` includes
the application's calls, locals, interrupt context, and compiler temporaries;
512 bytes is a comfortable starting value for small programs. `--min-os 8`
prevents an older kernel from starting code that assumes ABI 8.

## Put the process on a disk

XPROG can make a deterministic 16 MiB raw IDE image containing one 8.3 file:

```sh
bin/x/bin/xprog --esxdos --name HELLO.SYS build/my-yos/hello.sys \
  -o build/my-yos/hello.ide
```

The image has an MBR and a bootable FAT16 partition beginning at sector 2048.
Attach it to an emulator as a divIDE IDE disk. `--esxdos` accepts any nonempty
file, so the same command can package data or several independently prepared
test images. The current image builder intentionally creates one root file;
use normal host FAT tools when a disk needs several files.

YOS boots `shell.sys`. To replace the shell for a test, use the same process
payload but give the disk file that name:

```sh
bin/x/bin/xprog --esxdos --name SHELL.SYS build/my-yos/hello.sys \
  -o build/my-yos/boot.ide
```

Boot `bin/y/z80/spectrum/bin/yos-kernel.rom` with that disk and an
esxDOS-compatible divIDE configuration.

## Global and static objects

The backend supports zero-initialized and initialized C storage even though a
process can be placed anywhere in RAM:

```c
static unsigned calls;          /* cleared by the CRT */
static unsigned limit = 10;     /* copied by the CRT */
```

Never bake application addresses into external disk data. XL relocation fixes
code and data references when the loader chooses the process address.

## Returning from `main`

Returning calls the YOS `exit_process` operation. The process's threads,
owned heap blocks, image, stack, events, timers, and services are reclaimed by
the scheduler where ownership is recorded. ABI 8 has no parent-visible exit
code, so use files or a service protocol when another process needs a result.

Next: [Services and Console Output](SERVICES-AND-CONSOLE.md).
