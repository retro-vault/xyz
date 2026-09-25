# System Calls and Services

Traditional operating systems use a privileged trap instruction — a
software interrupt or `syscall` instruction — to transition from user
space into kernel space. The ZX Spectrum has no memory protection and no
privilege levels, so *yos* uses a lighter-weight mechanism instead:
**services**.

## What is a service?

A service is a named table of function pointers. Any piece of code that
wants to expose a public API registers itself as a service under a name.
Any other code that wants to use that API queries the service by name to
get the function table back, then calls functions straight through it.

| User program | | Kernel |
|---|---|---|
| `query_service("yos")` | → | returns `yos_t *` |
| `yos->read_key()` | → | `yos_t.read_key()` |

This is the *yos* equivalent of a system call table. The kernel registers
one service at boot:

| Name | Table | Header |
|---|---|---|
| `"yos"` | `yos_t` — kernel, drivers, filesystem, loader, and graphics, ABI version 6 | `y/include/yos.h` |

## Querying a service

An application obtains a table through `query_service`, which every
program's `crt0` implements as a two-byte `RST 0x18` stub:

```c
#include <yos.h>

/* Obtain the OS service table once, then keep it */
yos_t *yos = (yos_t *)query_service("yos");

uint8_t key = yos->read_key();
```

You only need to call `query_service` once per service, per process.
Store the pointer and reuse it — the lookup walks a linked list comparing
names, so it is not free.

## The `RST 0x18` mechanism

The ROM entry at `0x0018` jumps through slot 2 of the writable vector
table, which `main` points at `_svc_query_rst18` — a one-instruction
bridge into the kernel's `__svc_query`. RST 10 and the fixed 48K ROM print
entry at `0x09F4` share a wrapper that preserves HL and captures bytes
through a temporary RAM sink; the print handler checks the initialized
RAM-gate bytes before it ever touches that sink.

The calling convention is `sdcccall(1)`: the name pointer goes in `HL`,
and the table pointer (or zero) comes back in `DE`. Nothing is passed on
the stack.

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

`__svc_query` preserves `IX` and `IY`, and gives up after 256 list
entries, so a corrupted list can never hang the machine.

## The `yos_t` table

`yos_t` is an immutable 152-byte ROM table holding function pointers and
two data pointers, published directly at boot. The order in `yos.h` **is**
the ABI, and `yos.inc` gives assembly callers the same named byte
offsets. `yos->version()` returns 1. `yos->rom_model()` returns
`YOS_ROM_MODEL_48K`, `YOS_ROM_MODEL_128K`, or `YOS_ROM_MODEL_NEXT`,
depending on the model detected during boot. `yos->get_sys_info()` returns
a ROM descriptor whose fields point at the live process, thread, timer,
event, service, library-reference, and heap roots. The public read-only
object views use five-byte system headers with packed far owners. The loader
accepts images declaring the current pre-release ABI 1.

The three memory calls use packed far pointers (`bank,lo,hi`). The
command line passed to `exec_command` is NUL-terminated, excludes the
leading dot, and may contain an absolute path and arguments. A zero
return means success; failure is `0x100` plus the native esxDOS error
code. A print sink receives each character in A while interrupts are
masked, so it should do nothing more than buffer characters in RAM and
preserve the firmware's registers.

In summary:

| Group | Members |
|---|---|
| identity | `version`, `rom_model`, `get_sys_info`, `set_print_hook` |
| memory | `allocate_memory`, `free_memory`, `shrink_memory` |
| clock and critical sections | `clock_ticks`, `enter_critical_section`, `leave_critical_section` |
| timers and events | `create_timer`, `destroy_timer`, `create_event`, `destroy_event`, `set_event`, `wait_event` |
| threads | `create_thread`, `exit_thread`, `suspend_thread`, `resume_thread` |
| processes and libraries | `create_process`, `load_process`, `exit_process`, `load_library`, `process_load_error` |
| services | `query_service`, `register_service`, `unregister_service` |
| vectors | `get_interrupt_handler`, `set_interrupt_handler` |
| input | `read_key`, `calibrate_mouse`, `read_mouse` |
| esxDOS filesystem | `error_number` (pointer to the scheduler-virtualized per-thread errno cell), `open`, `close`, `read`, `write`, `lseek`, `fsync`, `unlink`, `rename`, `chdir`, `getcwd`, `mkdir`, `rmdir`, `stat`, `fstat`, `opendir`, `readdir`, `rewinddir`, `closedir`, `enumerate_disks` |
| commands | `exec_command` |
| graphics | `gpx_create`, `gpx_destroy`, `gpx_set_page`, `gpx_width`, `gpx_height`, `gpx_clear_screen`, `gpx_set_text_background`, `gpx_draw_pixel`, `gpx_draw_line`, `gpx_draw_bitmap`, `gpx_show_sprite`, `gpx_hide_sprite`, `gpx_draw_rectangle`, `gpx_fill_rectangle`, `gpx_measure_text`, `gpx_draw_text`, `gpx_get_system_font`, `gpx_get_tiny_font`, `gpx_get_stock_bitmap`, `gpx_draw_circle`, `gpx_fill_circle`, `gpx_draw_box` |

Assembly programs include `yos.inc` and use its byte offsets instead of
embedding raw slot numbers:

```asm
        .include "yos.inc"
        ld      hl,(_yos_table)
        ld      de,#YOS_OFFSET_REGISTER_SERVICE
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        call    ___sdcc_call_bc
```

Most entries map directly onto the kernel routine of the same meaning.
`allocate_memory`, `free_memory`, `shrink_memory`, and `create_timer` go
through small adapters (`kernel/_yos_malloc.s`, `_yos_free.s`,
`_yos_shrink.s`, `_yos_install_timer.s`) that supply kernel-private
arguments on their behalf. Public allocation searches every configured
banked user heap and returns a `yos_user_ptr_t`; free and shrink instead
use the bank already stored inside that pointer.
`shrink_memory(memory, size)` releases the bytes of a live block beyond
`size` whenever they can form a heap block of their own — the block
itself never moves. Standard C `malloc`, by contrast, uses a private
current-bank request, so its result remains a valid 16-bit near pointer.
Allocation and registration both use the current process — or, during
initialization, the library owner — while public timers remain
kernel-owned throughout.

Kernel objects returned by the table (`yos_thread_t`, `yos_process_t`,
`yos_event_t`, `yos_timer_t`, `yos_service_t`) are opaque handles. Their
internal layouts are described in the other chapters for the curious, but
applications must never depend on them.

Public shared-state transactions are protected, while stack-only
computation remains preemptible. Both fixed error cells are virtualized
per thread by the scheduler. Library loads use a separate whole-load
try-lock and return BUSY on contention. GPX returns independent
process-owned contexts, with protected framebuffer updates rather than a
single global drawing context — though that does not make caller-owned
buffers, service globals, or linked libc `errno` thread-local. See
[the application concurrency contract](../programming-yos/MEMORY-TIME-AND-CONCURRENCY.md)
for the precise guarantees.

## Filesystem drive paths

The first pathname passed to a filesystem operation may carry an optional
`A:` or `B:` prefix (case insensitive). This is a YOS/application naming
convention, not an esxDOS drive letter: the common adapter strips the
prefix and supplies native drive `0x40` for the DivIDE master, or `0x48`
for the slave. So `opendir("A:/")` supplies drive `0x40` and path `/`,
and a later `load_process("B:/TOOLS/APP.PRC")` selects the slave through
that same adapter. Unqualified paths keep the current esxDOS drive; none
of this remounts media or selects additional partitions. For `rename`,
the destination stays an unqualified path on the drive selected by the
first pathname — cross-drive rename is not provided. No service-table
entry or signature changes as a result of any of this.

`readdir` converts the native short-name record in this order:
attributes (one byte), ASCIIZ name, packed date/time (four bytes), size
(four bytes). The public `yos_directory_entry_t` contains `d_ino`,
`d_size`, `d_type`, `d_attributes`, and its 13-byte short-name array. The
returned entry is borrowed from its `yos_directory_t` object and gets
replaced by the next read — close the directory to release both the
firmware handle and the YOS allocation behind it.

## Registering a custom service

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

ABI 1 registrations belong to the current process and are reclaimed when
its last thread exits. During library initialization, they instead
belong to the library and stay unpublished until initialization succeeds.
Unregister an ordinary service explicitly if its interface becomes
invalid before that:

```c
yos->unregister_service(s);
```

Loader-managed library services must never be unregistered manually.
Clients acquire a library through `load_library`; calling `query_service`
alone does not retain it.

### Service name limit

Names are copied with the kernel's `__string_copy` into a fixed 16-byte
field, so a name may be at most 15 characters. Longer names are truncated
safely rather than rejected — even so, use distinct names of 15
characters or fewer.

## The service structure

Internally, each service is a 22-byte `service_t` system object on
`__sys_heap`:

```c
typedef struct service_s {
    sysobj_t hdr;               /*  0: list link + owner */
    char     name[16];          /*  4: "yos", "audio", ... */
    void    *fntable;           /* 20: pointer to your struct of fn ptrs */
} service_t;
```

Public services live in the `__svc_first` linked list
(`kernel/_svc_state.s`), newest first. `__svc_query` performs a linear
search by name using `__string_compare`.

## Graphics in `yos_t`

The ROM vendors libgpx `v1.1.0-1-g0ef6f07` (`y/src/z80/gpx/`, GPL-2.0; see
`gpx/README.md`). Its types, constants, and 24 entry points are appended
to the single ABI 1 `yos_t` table — there is no separate `gpx` service or
`gpx.h`. `gpx_create` allocates an independent six-byte context owned by
the current process (or the initializing library); `gpx_destroy` releases
it, and process cleanup catches any context an application forgot to
release itself.

```c
yos_t *yos = (yos_t *)query_service("yos");
gpx_t *screen = yos->gpx_create(GPXM_DEFAULT);
const font_t *font = yos->gpx_get_system_font();
yos->gpx_clear_screen();
yos->gpx_draw_text(screen, x, y, "hello", font, CO_FORE, BM_CPY, 0);
```

The Spectrum framebuffer itself is still shared. GPX protects byte-level
raster read/modify/write work and complete sprite save/show/hide
operations, but a compound drawing sequence can still interleave with
another thread or process mid-way through.

## Uses for custom services

**Shared hardware access.** Register a service that owns a resource —
the serial port, say — and serializes access to it. Every thread that
needs the port then goes through the service instead of touching the
hardware directly.

**Plug-in APIs.** A sound driver or file-format handler can be an XPRG
library with a relocated interface and an automatic reference lifetime.
A long-running active server, by contrast, may instead be a process that
registers a service of its own.

**Inter-process communication.** A service can expose a message queue or
shared buffer through functions like `send(msg)` and `recv()` — a
lightweight substitute for OS-level IPC.

**Dynamic libraries.** `load_library` loads XPRG service images,
relocates their code and interfaces, runs an optional self-registration
step once, and tracks per-client references. See [Libraries](LIBRARIES.md);
remember that name-only `query_service` is a borrowed lookup, not a
lifetime acquisition.

## No C library in ROM

The assembly kernel has no dependency on libc or the X runtime, and it
exports none either. A program built with `xcc` links whatever it needs
from `libc.a` and `libruntime.a` into its own XL image; the only things
it ever gets from the ROM are the tables returned by `query_service`.
