# System Calls and Services

Traditional operating systems use a privileged trap instruction (a software interrupt or `syscall` instruction) to transition from user space to kernel space. The ZX Spectrum has no memory protection and no privilege levels, so *yos* uses a lighter-weight mechanism: **services**.

## What Is a Service?

A service is a named table of function pointers. Any piece of code that wants to expose a public API registers itself as a service with a name. Any other code that wants to use that API queries the service by name to get the function table, then calls functions through it.

```
┌──────────────────┐         ┌────────────────────────────┐
│  user program    │ ──────► │  svc_query("yos")          │
│                  │         │  returns yos_t *           │
│  y->printf(...)  │ ──────► │  yos_t.printf(...)         │
└──────────────────┘         └────────────────────────────┘
```

This is the *yos* equivalent of a system call table. The OS itself registers a service called `"yos"` that exposes its public API.

## Querying the OS API

The fastest way to get the OS service pointer in C is through the `query_service` function, which is wired to `RST 0x10` (see below):

```c
#include <yos.h>

/* Obtain the OS service table */
yos_t *y = (yos_t *)query_service("yos");

/* Use it */
y->printf("Hello, world!\n");
y->clrscr();
y->setcur(true);
```

You only need to call `query_service` once per process. Store the pointer and reuse it — the lookup walks a linked list, so it is not free.

## The `RST 0x10` Mechanism

`query_service` is made available to *every* program — including assembly programs — through `RST 0x10`. The OS installs `_svc_query` as the handler for that vector during initialisation.

From assembly:

```asm
        ;; Push address of the service name string
        ld      hl, #service_name
        push    hl
        ;; RST 0x10 calls _svc_query(name)
        ;; Return value: HL = pointer to function table, or 0 if not found
        rst     0x10
        ;; HL now holds the function table pointer

service_name:
        .asciz  "yos"           ; null-terminated service name
```

This works because `RST 0x10` is a single-byte instruction — cheap in both code size and execution time — and the vector is hooked in RAM so it can be changed if needed.

## Registering a Custom Service

Any process can register its own service:

```c
#include <kernel/service.h>

/* Define your API */
typedef struct {
    void (*draw_pixel)(int x, int y);
    void (*clear_screen)();
} gpx_t;

/* Implement it */
static void my_draw_pixel(int x, int y) { /* ... */ }
static void my_clear_screen()           { /* ... */ }

static gpx_t gpx_api = {
    .draw_pixel    = my_draw_pixel,
    .clear_screen  = my_clear_screen,
};

/* Register it under the name "gpx" */
service_t *s = svc_register("gpx", &gpx_api);
```

Once registered, any other process can use it:

```c
gpx_t *g = (gpx_t *)query_service("gpx");
g->draw_pixel(10, 20);
```

### Important: service lifetime

A service is only valid while the process that registered it is running. If that process exits without calling `svc_unregister`, the entry remains in the service list but its function table points to freed memory — calling through it will crash. Always unregister before exiting:

```c
svc_unregister(s);
```

### Service name limit

Service names are stored in a fixed 16-byte field (`MAX_SVC_NAME_LEN = 16`), including the null terminator. Names longer than 15 characters will be silently truncated by `strcpy`.

## The Service Structure

Internally, each service is a `service_t` system object:

```c
typedef struct service_s {
    sysobj_t hdr;                   /* list link + owner */
    char     name[MAX_SVC_NAME_LEN];/* "yos", "gpx", etc. */
    void    *fntable;               /* pointer to your struct of fn ptrs */
} service_t;
```

All registered services are kept in the `_svc_first` linked list. `_svc_query` does a linear search by name using `strcmp`.

## Uses for Custom Services

Custom services are a flexible building block. Some ideas:

**Shared hardware access.** Register a service that owns a resource (e.g., the serial port) and serialises access to it. All threads that need the port go through the service instead of accessing the hardware directly.

**Plug-in APIs.** A graphics driver, a sound driver, or a file system can be loaded as a process and register a service. Other programs discover it at runtime without needing to be linked against it.

**Inter-process communication.** A service can expose a message queue or shared buffer with functions like `send(msg)` and `recv()`. This is a lightweight substitute for OS-level IPC.

**Dynamic linking.** Because a function table is just a struct of pointers, a "library" process can register its exported functions as a service. The dynamic linker is just `query_service`.

## Partial Standard C Library

*Yos* includes a subset of the standard C library built into the ROM image, covering parts of `<string.h>`, `<ctype.h>`, `<stdio.h>`, and `<time.h>`. Programs linked against *yos* can use these functions without reimplementing them.

Functions available include (but are not limited to):

- `strlen`, `strcpy`, `strcmp`, `strncmp`, `memset`, `memcpy`
- `isdigit`, `isalpha`, `toupper`, `tolower`
- `printf` (subset — via the `yos` service as `y->printf`)
- `clock` (via `<time.h>`)

This matters on a machine with 48 KB of RAM: duplicating the standard library in every program would be wasteful. Sharing one copy from ROM saves precious user memory.

## Resident Processes

Some services need to be available from the moment the OS starts. These are implemented as **resident processes** — they are loaded at fixed addresses near the top of physical memory and started before any user program runs. Because they live at fixed addresses, relocation is not needed and other programs can call them directly if they know the address. Resident processes are an advanced feature intended for OS extensions such as filesystem drivers.
