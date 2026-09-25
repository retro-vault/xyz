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
    yos_t *yos = (yos_t *)query_service("yos");
    return (!yos || yos->version() < YOS_VERSION) ? 1 : 0;
}
```

Compile and package it:

```sh
mkdir -p build/my-yos bin/y/examples
bin/x/bin/xcc -Os --platform=yos hello.c -o build/my-yos/hello.xl
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 6 \
  build/my-yos/hello.xl -o bin/y/examples/hello.prc
bin/x/bin/xprog --inspect bin/y/examples/hello.prc
```

`--name` is the process name stored in XPRG metadata. `--stack-size` includes
the application's calls, locals, compiler temporaries and library initializers;
the loader adds the 22-byte interrupt context separately. Thus
512 bytes is a comfortable starting value for small programs. `--min-os 6`
selects the unified table layout. ABI 6 deliberately rejects older images
whose table offsets are incompatible; rebuild applications against the
matching headers.

## Put the process on a disk

XPROG can make a deterministic 16 MiB raw IDE image containing one 8.3 file:

```sh
bin/x/bin/xprog --esxdos --name HELLO.PRC bin/y/examples/hello.prc \
  -o bin/y/examples/hello.ide
```

The image has an MBR and a bootable FAT16 partition beginning at sector 2048.
Attach it to an emulator as a divIDE IDE disk. `--esxdos` accepts any nonempty
file, so the same command can package data or several independently prepared
test images. The current image builder intentionally creates one root file;
use normal host FAT tools when a disk needs several files.

YOS boots `shell.sys`. To replace the shell for a test, use the same process
payload but give the disk file that name:

```sh
bin/x/bin/xprog --esxdos --name SHELL.SYS bin/y/examples/hello.prc \
  -o bin/y/examples/boot.ide
```

Boot `bin/y/arch/48/yos-kernel.rom` with that disk and an
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

Returning calls the YOS `exit_process` operation. After the last thread exits,
the scheduler reclaims the process image, stacks, process-owned heap blocks,
events and services, and releases its library references. Public timers are
kernel-owned and must be destroyed explicitly.

YOS does not record a parent or creator in `process_t`. A process loaded by
another process is independent after creation: there is no parent-visible
exit code, wait, or automatic notification. Use a file, event arranged by a
launcher, or a service protocol when another process needs a result.

Next: [Services and Console Output](SERVICES-AND-CONSOLE.md).
