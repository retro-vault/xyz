# Loadable Libraries

YOS ABI 1 loads relocatable XPRG service images as libraries. A library
owns one resident XL image and one service interface, but has no
scheduled thread and no private stack of its own — this is what lets it
avoid copying the same code into every process that uses it.

## Consuming a library

Describe the returned function-pointer table in a header shared by the
client and the library:

```c
#include <stdint.h>

typedef struct codec_api {
    uint16_t (* [[xcc::far]] version)(void);
    int (* [[xcc::far]] decode)(void *output, const void *input);
} codec_api_t;
```

Load it from a running process before calling an export:

```c
yos_t *yos = (yos_t *)query_service("yos");
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

Only the flags `YOS_LIBRARY_PRIVATE` (0) and `YOS_LIBRARY_SHARED` (1) are
valid. A library load requires a current process, since the reference it
creates needs an owner — called from kernel context, it returns
`NULL`/`YOS_PROCESS_LOAD_NO_PROCESS` instead.

The call completes synchronously, but a single nonblocking loader lock
covers both process and library loads. A competing or recursive call
returns immediately with `YOS_PROCESS_LOAD_BUSY`; retry later rather than
waiting with interrupts disabled. `process_load_error` is saved and
restored per thread, so another thread's loader result can never
overwrite yours. Never asynchronously terminate a thread while it is
inside a load or an initializer.

## Private or shared

`YOS_LIBRARY_PRIVATE` always loads and initializes a separate instance.
Its registration is not visible to `query_service` — use the returned
interface directly instead. Choose it for mutable global state that must
never be shared between clients.

`YOS_LIBRARY_SHARED` reuses a ready instance with the same complete
15-character XPRG name and image ABI byte. Code, initialized data,
BSS-like storage, initializer allocations, and mutable state are all
shared as a result, so exports must either be reentrant or take an
explicit per-client context whenever multiple threads can call them.

The path, the seven-character process display name, and the XPRG
descriptive ID are not sharing keys. YOS reads and validates the
descriptor before it looks for a matching resident instance; if one
exists, it neither reads nor relocates the payload, and it never reruns
the initializer.

## Lifetime

Every successful `load_library` call allocates a reference owned by the
calling process and increments the library's 16-bit reference count. Two
processes loading the same shared library therefore end up sharing one
resident image with two references against it. When one process's last
thread exits, its references are released, but the image stays resident
as long as the other reference remains. At zero references, YOS
unregisters the service and frees the library's image, its allocations,
its service, and its ownership object.

Repeated loads by the same process create repeated references. ABI 1 has
no explicit release/unload operation, so those references simply persist
until that process is reaped. Never free an interface, a library image,
or a loader-managed service yourself.

`query_service(name)` returns a borrowed pointer. It does not acquire a
reference, and it cannot distinguish private instances from
simultaneously resident ABIs. Use `load_library` whenever the caller
depends on the code staying resident.

## Export table and relocation

The service descriptor contains one three-byte `JP offset` record for
each export, in ABI slot order, followed by an ordinary XL payload
(version 2: header, code, then the relocation table). The YOS loader:

1. validates the descriptor, kind, YOS requirement, CRC, XL bounds,
   entry, relocation records, and every export target;
2. allocates and relocates the XL exactly as it would for a process
   image;
3. retains the relocated code and static storage in its selected bank,
   and allocates a common table of packed `{bank,address}` far function
   pointers;
4. creates a threadless library owner and transfers the image to it;
5. runs the optional relocated initializer once;
6. frees the temporary JP/XL header prefix and the trailing relocation
   table, publishes a successful shared registration, and attaches the
   client's reference.

The on-disk JP records are metadata only; clients receive the compact
three-byte far-function table instead. YOS supports 1–255 exports.
Fixed-load service images are rejected. Declare every slot with
`[[xcc::far]]` — XCC's YOS target invokes it through the RST28 gate.

## Initializer contract

If the XPRG descriptor has an entry, YOS calls that relocated entry
using `sdcccall(1)`:

```c
uint16_t initialize(void *bound_exports);
```

`HL` contains the loader-built export table. Return zero in `DE` for
success, or nonzero for `YOS_PROCESS_LOAD_INIT_ERROR`. Preserve `IX` and
`IY`. The initializer runs synchronously, on the loading thread's own
stack, so the client needs enough stack headroom for it and for any YOS
calls it makes along the way.

An initializer may simply return zero and let YOS register the bound
export table on its behalf. It may instead call `register_service` with
its own relocated interface table — in that case, both the interface
pointer and every function pointer inside it need XL relocation records,
and the registered name must equal the XPRG name exactly. YOS temporarily
assigns initializer allocations and registrations to the library, and
keeps registration private until the whole initialization succeeds;
failure rolls all of it back.

An initializer must return. It cannot recursively load a dependency,
because the nonblocking image-loader lock will simply report
`YOS_PROCESS_LOAD_BUSY`. Background threads or callbacks, dependency
chains, finalizers, and explicit unloading are all unsupported in ABI 1.
The ordinary process CRT is unsuitable here too, since it calls `main`
and exits; a C library instead needs a returning, library-specific
startup routine that initializes its own relocated C storage.

## Packaging a service

Link library objects as XL without the process CRT, retain a map, and
pass code-relative symbol values to `xprog` in table order. The shell
fixture is the reference recipe:

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
covering all four fixture exports. Put the `.svc` on the esxDOS drive
under an 8.3 filename. This follows the same working recipe used in
`y/src/z80/Makefile`.

## Working example and validation

`y/tests/shell-yos/shelllib.s` deliberately self-registers a relocated
embedded interface, allocates library-owned storage, and records
initializer and call counts. It is a build-only regression fixture,
independent of the minimal deployable shell.

Run the deterministic kernel suite with:

```sh
make -C y/src/z80 test
```

For a visible cold boot through real esxDOS firmware:

```sh
python3 y/tests/fuse/run.py --esxdos build/yos-fuse/esxdos089
```

The expected screen is the centred `Hello World!` greeting. See the
[Fuse runner guide](../../../tests/fuse/README.md) and the kernel-side
[Loadable Libraries](../the-book-of-yos/LIBRARIES.md) chapter for object
layouts, rollback order, and implementation details.

Next: [YOS API Reference](YOS-API-REFERENCE.md).
