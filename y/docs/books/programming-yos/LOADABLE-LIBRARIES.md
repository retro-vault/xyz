# Loadable Libraries

YOS ABI 1 loads relocatable XPRG service images as libraries. A library owns
one resident XL image and one service interface but has no scheduled thread or
private stack. This avoids copying the same code into every process.

## Consuming a library

Describe the returned function-pointer table in a header shared by the client
and library:

```c
#include <stdint.h>

typedef struct codec_api {
    uint16_t (*version)(void);
    int (*decode)(void *output, const void *input);
} codec_api_t;
```

Load it from a running process before calling an export:

```c
yos_t *yos = yos_get_api();
codec_api_t *codec = (codec_api_t *)yos->load_library(
    "codec.svc", YOS_LIBRARY_SHARED);
if (!codec) {
    enum yos_process_load_error why =
        (enum yos_process_load_error)*yos->process_load_error;
    return why;
}
if (codec->version() != 1)
    return 1;
```

Only flags `YOS_LIBRARY_PRIVATE` (0) and `YOS_LIBRARY_SHARED` (1) are valid.
A library load requires a current process because the reference needs an
owner. It therefore returns `NULL`/`YOS_PROCESS_LOAD_NO_PROCESS` in kernel
context.

The call completes synchronously, but a single nonblocking loader lock covers
process and library loads. A competing or recursive call returns immediately
with `YOS_PROCESS_LOAD_BUSY`; retry later rather than waiting with interrupts
disabled. `process_load_error` is saved/restored per thread, so another
thread's loader result cannot overwrite yours. Do not asynchronously terminate
a thread while it is inside a load or initializer.

## Private or shared

`YOS_LIBRARY_PRIVATE` always loads and initializes a separate instance. Its
registration is not visible to `query_service`; use the returned interface.
Choose it for mutable global state that must not be shared between clients.

`YOS_LIBRARY_SHARED` reuses a ready instance with the same complete
15-character XPRG name and image ABI byte. Code, initialized data, BSS-like
storage, initializer allocations, and mutable state are then shared. Exports
must be reentrant or take an explicit per-client context when multiple threads
can call them.

The path, seven-character process display name, and XPRG descriptive ID are
not sharing keys. YOS reads and validates the descriptor before looking for a
matching resident instance. If one exists, it does not read or relocate the
payload and does not rerun the initializer.

## Lifetime

Every successful `load_library` call allocates a reference owned by the
calling process and increments the library's 16-bit reference count. Two
processes loading the same shared library therefore retain one image with two
references. When one process's last thread exits, its references are released;
the image stays resident while the other reference remains. At zero, YOS
unregisters the service and frees the library's image, allocations, service,
and ownership object.

Repeated loads by one process create repeated references. ABI 1 has no
explicit release/unload operation, so those references remain until that
process is reaped. Never free an interface, library image, or loader-managed
service yourself.

`query_service(name)` returns a borrowed pointer. It does not acquire a
reference and cannot distinguish private instances or simultaneously resident
ABIs. Use `load_library` whenever the caller depends on the code remaining
resident.

## Export table and relocation

The service descriptor contains one three-byte `JP offset` record for each
export, in ABI slot order, followed by an ordinary XL payload. The YOS loader:

1. validates the descriptor, kind, YOS requirement, CRC, XL bounds, entry,
   relocation records, and every export target;
2. allocates and relocates the XL exactly like a process image;
3. converts the JP records in place into a compact array of absolute 16-bit
   function pointers;
4. creates a threadless library owner and transfers the image to it;
5. runs the optional relocated initializer once;
6. publishes a successful shared registration and attaches the client
   reference.

The on-disk JP records are metadata; clients receive the compact two-byte
function-pointer table. YOS supports 1–255 exports. Fixed-load service images
are rejected.

## Initializer contract

If the XPRG descriptor has an entry, YOS calls that relocated entry using
`sdcccall(1)`:

```c
uint16_t initialize(void *bound_exports);
```

`HL` contains the loader-built export table. Return zero in `DE` for success
or nonzero for `YOS_PROCESS_LOAD_INIT_ERROR`. Preserve `IX` and `IY`. The
initializer runs synchronously on the loading thread's stack, so the client
must have enough stack headroom for it and any YOS calls it makes.

An initializer may simply return zero and let YOS register the bound export
table. It may instead call `register_service` with its own relocated interface
table. In that case both the interface pointer and every function pointer
inside it must have XL relocation records, and the registered name must equal
the XPRG name. YOS temporarily assigns initializer allocations and
registrations to the library and keeps registration private until the whole
initialization succeeds. Failure rolls all of it back.

An initializer must return. It cannot recursively load a dependency because
the nonblocking image-loader lock reports `YOS_PROCESS_LOAD_BUSY`. Background
threads/callbacks, dependency chains, finalizers, and explicit unloading are
not supported in ABI 1. The ordinary process CRT is unsuitable because it
calls `main` and exits; a C library needs a returning library-specific startup
that initializes its relocated C storage.

## Packaging a service

Link library objects as XL without the process CRT, retain a map, and pass
code-relative symbol values to `xprog` in table order. The shell fixture is
the reference recipe:

```sh
mkdir -p build/my-yos bin/y/examples
bin/x/bin/xas --mode=sdcc y/tests/shell-yos/shelllib.s \
  -o build/my-yos/shelllib.rel
bin/x/bin/xld --mode=sdcc -f xl -nostdlib --no-default-runtime \
  -T bin/x/z80/lib/linker-yos.lk -Map=build/my-yos/shelllib.map \
  -o build/my-yos/shelllib.xl build/my-yos/shelllib.rel
bin/x/bin/xprog --service --name shelllib --abi 1 --min-os 1 \
  --entry 0x$(awk '$2 == "_entry" {print $1}' build/my-yos/shelllib.map) \
  --export 0x$(awk '$2 == "_probe" {print $1}' build/my-yos/shelllib.map) \
  --export 0x$(awk '$2 == "_message" {print $1}' build/my-yos/shelllib.map) \
  --export 0x$(awk '$2 == "_initializations" {print $1}' build/my-yos/shelllib.map) \
  --export 0x$(awk '$2 == "_calls" {print $1}' build/my-yos/shelllib.map) \
  build/my-yos/shelllib.xl -o bin/y/examples/shelllib.svc
```

The `awk` expressions extract code-relative offsets from the linker map,
including all four fixture exports. Put the `.svc` on the esxDOS drive using
an 8.3 filename. This follows the working recipe in `y/src/z80/Makefile`.

## Working example and validation

`y/tests/shell-yos/shelllib.s` deliberately self-registers a relocated
embedded interface, allocates library-owned storage, and records initializer
and call counts. `shell.sys` loads it shared and draws the string returned by
the relocated `message` export.

Run the deterministic kernel suite with:

```sh
make -C y/src/z80 test
```

For a visible cold boot through real esxDOS firmware:

```sh
python3 y/tests/fuse/run.py --esxdos build/yos-fuse/esxdos089
```

The expected screen is the centred Alto greeting with `Library OK` below it.
See the [Fuse runner guide](../../../tests/fuse/README.md) and the kernel-side
[Loadable Libraries](../the-book-of-yos/LIBRARIES.md) chapter for object
layouts, rollback order, and implementation details.
