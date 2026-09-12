# System Calls and Services

Traditional operating systems use a privileged trap instruction (a software interrupt or `syscall` instruction) to transition from user space to kernel space. The ZX Spectrum has no memory protection and no privilege levels, so *yos* uses a lighter-weight mechanism: **services**.

## What Is a Service?

A service is a named table of function pointers. Any piece of code that wants to expose a public API registers itself as a service with a name. Any other code that wants to use that API queries the service by name to get the function table, then calls functions through it.

```
┌──────────────────────┐         ┌────────────────────────────┐
│  user program        │ ──────► │  query_service("yos")      │
│                      │         │  returns yos_t *           │
│  yos->read_key()     │ ──────► │  yos_t.read_key()          │
└──────────────────────┘         └────────────────────────────┘
```

This is the *yos* equivalent of a system call table. The kernel registers two services at boot:

| Name | Table | Header |
|---|---|---|
| `"yos"` | `yos_t` — kernel, drivers and filesystem, ABI version 1 | `y/include/yos.h` |
| `"gpx"` | `gpx_api_t` — the complete libgpx drawing API (24 entries) | `y/include/gpx.h` |

## Querying a Service

An application obtains a table through `query_service`, which every program's `crt0` implements as a two-byte `RST 0x18` stub:

```c
#include <yos.h>

/* Obtain the OS service table once, then keep it */
yos_t *yos = (yos_t *)query_service("yos");

uint8_t key = yos->read_key();
```

You only need to call `query_service` once per service per process. Store the pointer and reuse it — the lookup walks a linked list comparing names, so it is not free.

## The `RST 0x18` Mechanism

The ROM entry at `0x0018` jumps through slot 2 of the writable vector table, which `main` points at `_svc_query_rst18` — a one-instruction bridge to the kernel's `__svc_query`. RST 10 is not used because it must remain the immediate `RET` required while esxDOS cold-boots a replacement ROM.

The calling convention is `sdcccall(1)`: the name pointer goes in `HL` and the table pointer (or zero) comes back in `DE`. Nothing is passed on the stack.

```asm
        ;; C-callable query_service(const char *name)
_query_service::
        rst     0x18                    ; HL = name in, DE = table out
        ret

        ;; direct use from assembly
        ld      hl, #service_name
        rst     0x18
        ld      a, d
        or      e
        jr      z, .not_found           ; DE == 0: no such service
        ;; DE now holds the function table pointer

service_name:
        .asciz  "yos"
```

`__svc_query` preserves `IX` and `IY`, and gives up after 256 list entries so a corrupted list cannot hang the machine.

## The `yos_t` Table

`yos_t` is a 96-byte struct of 46 function pointers and two data pointers,
built at boot by `__syscall_table_init` from an ordered template in ROM. The
order of the members in `yos.h` **is** the ABI; `yos->version()` returns
`YOS_VERSION` (currently 1). This is a clean numbering baseline for the
complete current table, not the earlier incremental development numbering.
In summary:

| Group | Members |
|---|---|
| identity and memory | `version`, `allocate_memory`, `free_memory` |
| clock and critical sections | `clock_ticks`, `enter_critical_section`, `leave_critical_section` |
| timers and events | `create_timer`, `destroy_timer`, `create_event`, `destroy_event`, `set_event` |
| threads and processes | `create_thread`, `exit_thread`, `suspend_thread`, `resume_thread`, `create_process`, `exit_process` |
| services and vectors | `query_service`, `register_service`, `unregister_service`, `get_interrupt_handler`, `set_interrupt_handler` |
| input | `read_key`, `calibrate_mouse`, `read_mouse` |
| esxDOS filesystem | `error_number` (pointer to the scheduler-virtualized per-thread errno cell), `open`, `close`, `read`, `write`, `lseek`, `fsync`, `unlink`, `rename`, `chdir`, `getcwd`, `mkdir`, `rmdir`, `stat`, `fstat`, `opendir`, `readdir`, `rewinddir`, `closedir`, `enumerate_disks` |
| loader | `load_process`, `process_load_error` (per-thread status byte), `load_library` |

Most entries map directly onto the kernel routine of the same meaning. `allocate_memory`, `free_memory` and `create_timer` go through small adapters (`kernel/_yos_malloc.s`, `_yos_free.s`, `_yos_install_timer.s`) that supply kernel-private arguments. Allocation and registration use the current process (or initialization's library owner); public timers remain kernel-owned.

Kernel objects returned by the table (`yos_thread_t`, `yos_process_t`, `yos_event_t`, `yos_timer_t`, `yos_service_t`) are opaque handles; their layouts are described in the other chapters for the curious, but applications must not depend on them.

Public shared-state transactions are protected, while stack-only computation
can be preempted. Both fixed error cells are virtualized per thread by the
scheduler. Library loads use a separate whole-load try-lock and return BUSY
on contention. GPX returns independent process-owned contexts, with protected
framebuffer updates rather than a global drawing context. This does not make
caller-owned buffers, service globals, or linked libc `errno` thread-local;
see [the application concurrency contract](../programming-yos/MEMORY-TIME-AND-CONCURRENCY.md).

## Registering a Custom Service

Any process can register its own service:

```c
#include <yos.h>

/* Define your API */
typedef struct {
    void (*beep)(uint8_t pitch);
    void (*click)(void);
} snd_t;

/* Implement it */
static void my_beep(uint8_t pitch) { /* ... */ }
static void my_click(void)         { /* ... */ }

static snd_t snd_api = { my_beep, my_click };

/* Register it under the name "snd" */
yos_service_t *s = yos->register_service("snd", &snd_api);
```

Once registered, any other process can use it:

```c
snd_t *snd = (snd_t *)query_service("snd");
snd->click();
```

### Service lifetime

ABI 1 registrations belong to the current process and are reclaimed when its
last thread exits. During library initialization they belong to the library
and remain unpublished until initialization succeeds. Unregister an ordinary
service explicitly if its interface becomes invalid earlier:

```c
yos->unregister_service(s);
```

Loader-managed library services must not be manually unregistered. Clients acquire a library with `load_library`; `query_service` alone does not retain it.

### Service name limit

Names are copied with the kernel's `__string_copy` into a fixed 16-byte field, so a name may be at most 15 characters. Longer names are now truncated safely; use distinct names of at most 15 characters.

## The Service Structure

Internally, each service is a 22-byte `service_t` system object on `__sys_heap`:

```c
typedef struct service_s {
    sysobj_t hdr;               /*  0: list link + owner */
    char     name[16];          /*  4: "yos", "gpx", ... */
    void    *fntable;           /* 20: pointer to your struct of fn ptrs */
} service_t;
```

Public services are kept in the `__svc_first` linked list (`kernel/_svc_state.s`), newest first. `__svc_query` does a linear search by name with `__string_compare`.

## The `gpx` Service

The ROM vendors libgpx `v1.1.0-1-g0ef6f07` (`y/src/z80/gpx/`, GPL-2.0, see `gpx/README.md`) and registers its function table as `"gpx"`. The `gpx_api_t` in `gpx.h` covers drawing contexts, pixels, lines, rectangles and selected-edge boxes, circles, polygons, text with the system and tiny fonts, bitmaps, sprites and page selection. The `draw_box` entry is appended after the original 23 slots, preserving their ABI offsets. Its eight bytes of writable state live in `_INITIALIZED` like any other kernel data. The boot-time shell (`y/tests/shell-yos/shell.c`) is a minimal example:

```c
gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
gpx_t *screen = gpx->create(GPXM_DEFAULT);
const font_t *font = gpx->get_system_font();
gpx->clear_screen();
gpx->draw_text(screen, x, y, "hello", font, CO_FORE, BM_CPY, 0);
```

## Uses for Custom Services

**Shared hardware access.** Register a service that owns a resource (e.g., the serial port) and serialises access to it. All threads that need the port go through the service instead of accessing the hardware directly.

**Plug-in APIs.** A sound driver or file-format handler can be an XPRG
library with a relocated interface and automatic reference lifetime. A
long-running active server may instead be a process that registers a service.

**Inter-process communication.** A service can expose a message queue or shared buffer with functions like `send(msg)` and `recv()`. This is a lightweight substitute for OS-level IPC.

**Dynamic libraries.** `load_library` loads XPRG service images, relocates their code and interfaces, runs optional self-registration once, and tracks per-client references. See [Libraries](LIBRARIES.md); name-only `query_service` is a borrowed lookup, not a lifetime acquisition.

## No C Library in ROM

The assembly kernel has no dependency on libc or the X runtime, and it exports none. A program built with `xcc` links whatever it needs from `libc.a` and `libruntime.a` into its own XL image; the only things it gets from the ROM are the tables returned by `query_service`.
