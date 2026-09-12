# Loadable Libraries

ABI 1 includes `load_library(path, flags)` in `yos_t`. It loads an XPRG
service (`xprog --service`) and returns a direct function-pointer table:

```c
shelllib_api_t *library = yos->load_library(
    "shelllib.svc", YOS_LIBRARY_SHARED);
if (library && library->probe() == SHELLLIB_RESULT)
    /* the relocated export was called successfully */;
```

The complete working example is `y/tests/shell-yos/shelllib.s`, with the
matching interface in `shelllib.h`. `shell.sys` loads this library and
displays its returned "Library OK" string. Copy **both** `shell.sys` and
`shelllib.svc` from `bin/y/z80/spectrum/bin/` to the esxDOS drive.

## Relocate, Initialize, Publish

Process and library loading enter `kernel/_image_load.s`. They share disk
reads, descriptor checks, image allocation, CRC checking, entry validation
and the existing XL relocator. No second relocation implementation exists.

For a new library:

1. Allocate one block containing the export metadata and complete XL.
2. Check the XL CRC and relocate every XL relocation record in place.
3. Validate each three-byte XPRG `JP offset` export and compact the targets
   into a two-byte-per-slot, absolute YOS function-pointer table. The
   on-disk JP table is **not** itself a C function-pointer array.
4. Create a library ownership object, without a thread or stack, and
   transfer the image to it.
5. If XPRG has an entry, call that **relocated** initializer once, using the
   loading thread's stack. Its `sdcccall(1)` contract is
   `uint16_t initialize(void *exports)`: HL receives the bound table,
   DE returns zero on success, nonzero on failure; IX/IY are preserved.
6. The initializer may call `register_service` with its own interface.
   Both that interface's address and its embedded function addresses must
   have XL relocation records. The fixture deliberately publishes its
   **own embedded table**, exercising both kinds of relocation.
7. If initialization did not register a service (or no initializer exists),
   register the loader's bound export table. The service name must match
   the full XPRG name and its interface must be non-null.
8. Publish a shared service only after initialization succeeds and attach
   the caller's reference. Private registrations remain out of global
   lookup.

During initialization, `thread.hdr.owner` temporarily overrides the owner
used by `allocate_memory` and `register_service`. Both become library-owned.
Registration is staged in `__library_private_services`, not the public
list. The real `thread.process` is never changed: another thread exiting
cannot make cleanup mistake the loading client for a dead process. The
override is restored on success and failure.

Failure closes the file and frees the image, library object, staged
services, initializer allocations and any uncommitted reference. An
initializer must return; it must not exit its thread, create background
callbacks/threads, or retain resources outside this ownership contract.
The ordinary YOS **process CRT is not a library CRT**: it calls main and
exits. A C library needs a returning initializer/startup that also
initializes its C storage. The assembly fixture contains its initialized
storage directly in XL and needs no process CRT.

## Sharing and Lifetime

`YOS_LIBRARY_PRIVATE` (0) always creates a separate instance.
`YOS_LIBRARY_SHARED` (1) reuses a ready shared instance matching the full
15-character XPRG name and image ABI byte. The seven-character process
display name and descriptive XPRG ID hash are not sharing keys. Different
paths with that same identity reuse the resident image; the resident
version wins until its last reference disappears. Reuse reads and validates
the descriptor but does not read/relocate the payload or run initialization
again.

Every successful call creates one six-byte reference object:

```c
struct library_reference {
    sysobj_t hdr;           /* next + owner = acquiring client process */
    process_t *library;
};
```

That costs 13 system-heap bytes including the seven-byte allocation header,
before allocator slack. Repeated loads by one process count separately.
There is no manual unload API in this first version: all references are
released when the client's **last** thread is reclaimed.

The library reuses the 15-byte process object:

| Offset | Library meaning |
|---:|---|
| 0 | next process/library |
| 2 | null owner (not the first loading client) |
| 4 | 1 = private library, 3 = shared library; normal processes use 0 |
| 5 | service object pointer |
| 7 | image ABI |
| 8–12 | unused |
| 13 | 16-bit reference count, instead of main-thread pointer |

The service holds the full name and interface pointer. At zero references,
`process_reap` unregisters owned services and frees library-owned memory
and the object. A nonzero count protects threadless libraries from reaping.
Ordinary process objects and thread objects have not grown.

`query_service` returns a **borrowed** pointer and does not acquire a
reference. Load a dynamic library before using it, and never manually free
its image or unregister its loader-managed service. Use the returned
interface to select a particular ABI or private instance; name-only query
cannot distinguish simultaneously loaded ABIs. Shared mutable state is
shared, so exports must be reentrant or accept per-client contexts.

## Limits and Errors

The loader supports relocatable services with 1–255 exports. Fixed-address
service JP stubs are not this YOS binding and are rejected. Library
dependencies, finalizers and explicit unloading are not implemented.
A nonblocking loader lock serializes loads; competing or recursive loads
return BUSY without disabling interrupts for the whole load or initializer.
The fixed loader-status cell is saved/restored per thread, so a competing
call's BUSY result cannot overwrite the initiating thread's result. Services
are only published after initialization. Mutable state inside a shared
library still needs its own synchronization or per-client contexts.
Each native esxDOS call does mask interrupts until divIDE restores the ROM
containing the scheduler; validation, CRC and relocation can be preempted.

Both loading APIs report through `process_load_error`. Existing codes
0–8 remain stable; code 6 means wrong image kind for the selected loader.
ABI 1 defines:

| Code | Meaning |
|---:|---|
| 9 | BUSY — another load is in progress |
| 10 | NO_PROCESS — library loading needs a current client process |
| 11 | INIT_ERROR — initializer returned a nonzero status |

The emulator suite executes the actual packaged shell and library through
the ROM filesystem and loader, with short reads and deliberately clobbered
firmware registers. It checks self-registration after relocation, staged
publication, shared/private state, distinct ABIs, surviving sibling threads,
last-reference cleanup, malformed metadata/XL/exports, CRC/read errors,
initializer rollback, and exhausted image/object/reference heaps.
