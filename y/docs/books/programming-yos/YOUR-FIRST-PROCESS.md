# Your First Process

Build X and YOS once from the repository root:

```sh
make -C x
make -C y/src/z80
```

The compiler is `bin/x/bin/xcc`; the operating-system platform is selected
with `--platform=yos`. Start with `hello.c`:

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
bin/x/bin/xprog --process --name hello --stack-size 512 --min-os 1 \
  build/my-yos/hello.xl -o bin/y/examples/hello.prc
bin/x/bin/xprog --inspect bin/y/examples/hello.prc
```

`--name` is the process name stored in XPRG metadata. `--stack-size`
covers the application's calls, locals, compiler temporaries, and library
initializers — the loader adds the 22-byte interrupt context separately,
so 512 bytes is a comfortable starting value for small programs.
`--min-os 1` selects the current pre-release unified table layout. Rebuild
applications against the matching headers whenever that layout moves.

## Put the process on a disk

`xprog` can build a deterministic 16 MiB raw IDE image containing one
8.3 file:

```sh
bin/x/bin/xprog --esxdos --name HELLO.PRC bin/y/examples/hello.prc \
  -o bin/y/examples/hello.ide
```

The image carries an MBR and a bootable FAT16 partition beginning at
sector 2048. Attach it to an emulator as a divIDE IDE disk. `--esxdos`
accepts any nonempty file, so the same command can package data or
several independently prepared test images. The current image builder
intentionally creates just one root file — reach for ordinary host FAT
tools when a disk needs several.

YOS boots `shell.sys`. To replace the shell for a test, package the same
process payload but give the disk file that name instead:

```sh
bin/x/bin/xprog --esxdos --name SHELL.SYS bin/y/examples/hello.prc \
  -o bin/y/examples/boot.ide
```

Boot `bin/y/arch/48/yos-kernel.rom` with that disk and an
esxDOS-compatible divIDE configuration.

## Running your program

`make -C y` (not just `make -C y/src/z80`) builds the full Spectrum
release under `bin/y/`, complete with launchers that spare you from
configuring an emulator and disk media by hand. Replace the shipped shell
as shown above, then run:

```sh
bin/y/run-48.sh --prepare-only   # print the emulator command without opening one
bin/y/run-48.sh                  # build private media and start ZEsarUX
bin/y/run-128.sh
bin/y/run-next.sh
```

Each launcher wraps `bin/y/scripts/run-yos.py`: it stages a private
esxDOS disk image (`--work-dir DIR` to choose where), bundles esxDOS 0.8.9
(`--esxdos DIR` or `ESXDOS` to override it), and starts ZEsarUX.
`--headless` additionally requires that the packaged shell actually
render before exiting, which is useful for scripted checks. See
`y/distribution/SPECTRUM.md` for the complete option list.

Run these from the built `bin/y/` tree, not from `y/scripts/` in the
source tree. `run-48.sh` execs `scripts/run-yos.py` (hyphenated) relative
to its own directory, and that hyphenated name only exists once
`make -C y` stages it there from `y/scripts/run_yos.py` (underscored).
The underscored source script still works when run directly, with
explicit `--rom`/`--shell` paths — the staged launchers are simply the
documented, no-argument way to reach it.

For a visible cold boot through real esxDOS firmware, straight from the
source tree, see [the Fuse runner guide](../../../tests/fuse/README.md)
and `python3 y/tests/fuse/run.py`.

## Global and static objects

The backend supports both zero-initialized and initialized C storage,
even though a process can be placed anywhere in RAM:

```c
static unsigned calls;          /* cleared by the CRT */
static unsigned limit = 10;     /* copied by the CRT */
```

Never bake application addresses into external disk data. XL relocation
fixes up code and data references once the loader has chosen the
process's actual address.

## Returning from `main`

Returning calls the YOS `exit_process` operation. After the last thread
exits, the scheduler reclaims the process image, its stacks, its
process-owned heap blocks, its events and services, and releases its
library references. Public timers are kernel-owned, though, and must be
destroyed explicitly.

YOS records no parent or creator in `process_t`. A process loaded by
another process is fully independent after creation — there is no
parent-visible exit code, no wait, and no automatic notification. Use a
file, an event arranged by a launcher, or a service protocol whenever
another process needs a result back.

Next: [Services and Console Output](SERVICES-AND-CONSOLE.md).
