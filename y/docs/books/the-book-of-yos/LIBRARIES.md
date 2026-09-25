# Loadable Libraries

ABI 1 includes `load_library(path, flags)` in `yos_t`. It loads an XPRG
service (`xprog --service`) and returns a fixed-memory table of far
function pointers:

```c
shelllib_api_t *library = yos->load_library(
    "shelllib.svc", YOS_LIBRARY_SHARED);
if (library && library->probe() == SHELLLIB_RESULT)
    /* the relocated export was called successfully */;
```

The complete regression fixture is `y/tests/shell-yos/shelllib.s`. It is
built only under `build/yos-z80/apps/` by the test target — the deployable
Hello World shell deliberately has no library dependency at all.

## Relocate, initialize, publish

Process and library loading both enter `kernel/_image_load.s`. They share
disk reads, descriptor checks, temporary metadata allocation, CRC
checking, entry validation, and the existing XL relocator — there is no
second relocation implementation anywhere.

For a new library:

1. Allocate a temporary block containing the export metadata and the
   complete XL image (version 2: header, code, then the relocation
   table).
2. Check the XL CRC and relocate the code in place, inside its existing
   read buffer. No second resident-code allocation or copy is made.
3. Validate each three-byte XPRG `JP offset` export and compact the
   targets into a three-byte-per-slot `{bank,address}` YOS far-function
   table in common memory. The on-disk JP table is **not** itself a C
   function-pointer array.
4. Shrink the bank allocation to the code end, freeing the now-consumed
   relocation table. Create a library ownership object with no thread or
   stack, and transfer the retained code and the separate common table to
   it; the loader's final cleanup frees the leading metadata block.
   Splitting preserves ownership and allocation flags throughout, all
   under a critical section, and export validation always finishes before
   any metadata is released.
5. If the XPRG carries an entry, call that **relocated** initializer
   exactly once, on the loading thread's own stack. Its `sdcccall(1)`
   contract is `uint16_t initialize(void *exports)`: HL receives the
   bound table, DE returns zero on success and nonzero on failure, and
   IX/IY are preserved.
6. The initializer may call `register_service` with its own interface.
   Both that interface's address and its embedded function addresses need
   XL relocation records. The fixture deliberately publishes its **own
   embedded table**, so it exercises both kinds of relocation at once.
7. If initialization did not register a service — or no initializer
   exists at all — the loader registers its own bound export table
   instead. The service name must match the full XPRG name, and its
   interface must be non-null.
8. A shared service is published only after initialization succeeds, at
   which point the caller's reference is attached. Private registrations
   never enter the global lookup.

During initialization, `thread.hdr.owner` temporarily overrides the owner
used by `allocate_memory` and `register_service`; both become
library-owned for the duration. Registration is staged in
`__library_private_services`, not the public list. The real
`thread.process` field is never touched, so another thread exiting cannot
make cleanup mistake the loading client for a dead process. The override
is restored afterward, whether initialization succeeded or failed.

Failure closes the file and frees the image, the library object, any
staged services, initializer allocations, and any uncommitted reference.
An initializer must return — it must not exit its own thread, create
background callbacks or threads, or retain resources outside this
ownership contract. The ordinary YOS **process CRT is not a library
CRT**: it calls `main` and exits. A C library instead needs a returning
initializer/startup routine that also initializes its own C storage. The
assembly fixture carries its initialized storage directly in the XL
image, so it needs no process CRT at all.

## Sharing and lifetime

`YOS_LIBRARY_PRIVATE` (0) always creates a separate instance.
`YOS_LIBRARY_SHARED` (1) reuses a ready shared instance that matches the
full 15-character XPRG name and image ABI byte. Neither the
seven-character process display name nor the descriptive XPRG ID hash is
a sharing key. Different paths that resolve to the same identity reuse
the one resident image — that resident version wins until its last
reference disappears. Reuse still reads and validates the descriptor, but
it does not read or relocate the payload, and it does not run
initialization again.

Every successful call creates one six-byte reference object:

```c
struct library_reference {
    sysobj_t hdr;           /* next + owner = acquiring client process */
    process_t *library;
};
```

That costs 13 system-heap bytes, including the seven-byte allocation
header, before allocator slack. Repeated loads by the same process are
counted separately. There is no manual unload API in this first version —
every reference is released only when the client's **last** thread is
reclaimed.

The library reuses the same 16-byte process object:

| Offset | Library meaning |
|---:|---|
| 0 | next process/library |
| 2 | null owner (not the first loading client) |
| 4 | 1 = private library, 3 = shared library; normal processes use 0 |
| 5 | service object pointer |
| 7 | image ABI |
| 8–12 | unused |
| 13 | 16-bit reference count, instead of main-thread pointer |
| 15 | logical bank containing the resident code |

The service object holds the full name and interface pointer. At zero
references, `process_reap` unregisters owned services and frees the
library-owned memory along with the object itself. A nonzero count
protects threadless libraries from being reaped prematurely. Both the
stable common export table and the banked image are library-owned.

`query_service` returns a **borrowed** pointer and does not acquire a
reference on its own. Load a dynamic library before using it, and never
manually free its image or unregister its loader-managed service. Use the
returned interface to select a particular ABI or private instance —
name-only query cannot distinguish between simultaneously loaded ABIs.
Shared mutable state really is shared, so exports must either be
reentrant or accept per-client contexts.

## Limits and errors

The loader supports relocatable services with 1–255 exports.
Fixed-address service JP stubs are not this YOS binding, and are
rejected. Library dependencies, finalizers, and explicit unloading are
not implemented. A nonblocking loader lock serializes loads: competing or
recursive loads return BUSY without disabling interrupts for the whole
load or its initializer. The fixed loader-status cell is saved and
restored per thread, so a competing call's BUSY result can never overwrite
the initiating thread's own result. Services are published only after
initialization completes. Mutable state inside a shared library still
needs its own synchronization, or per-client contexts, regardless.
Descriptor-backed filesystem calls keep their complete descriptor/native
I/O transaction protected, with the native esxDOS gate nested inside it —
this can delay scheduling during disk work, but the loader lock itself
does not disable interrupts, so validation, CRC, relocation, and
initialization can all still be preempted. Never asynchronously terminate
a thread while a load or initializer is active; abandoning its stack
frame would also abandon the loader lock and its resources.

Both loading APIs report through `process_load_error`. The existing codes
0–8 remain stable; code 6 means the selected loader received the wrong
image kind. ABI 1 defines:

| Code | Meaning |
|---:|---|
| 9 | BUSY — another load is in progress |
| 10 | NO_PROCESS — library loading needs a current client process |
| 11 | INIT_ERROR — initializer returned a nonzero status |

The emulator test suite exercises the actual packaged shell and library
through the ROM filesystem and loader, with short reads and deliberately
clobbered firmware registers. It checks self-registration after
relocation, staged publication, shared/private state, distinct ABIs,
surviving sibling threads, last-reference cleanup, malformed
metadata/XL/exports, CRC/read errors, initializer rollback, exhausted
image/object/reference heaps, RST20/RST28 register and stack behavior, and
a compiler-generated far call with a callee-cleaned stack argument.
