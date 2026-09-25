# Services and Console Output

YOS uses named tables of function pointers instead of a fixed list of call
numbers. This permits optional and independently versioned facilities.

## The one primitive: `query_service`

```c
#include <yos.h>

yos_t *yos = (yos_t *)query_service("yos");
```

The function executes `RST 0x18` with the service name and returns the table,
or `NULL` when no service has that name. The comparison is case-sensitive.
Always test an optional service before dereferencing it.

`YOS_VERSION` is the ABI required by the installed header, currently 6. XPRG's
`--min-os 6` is the loader-side check. An application can additionally check
the returned table's `version()` for diagnostics and unusual launchers.

Graphics is part of the same `yos_t` table:

```c
yos_t *yos = (yos_t *)query_service("yos");
if (yos) {
    gpx_t *screen = yos->gpx_create(GPXM_DEFAULT);
    if (screen)
        yos->gpx_draw_pixel(screen, 20, 20, CO_FORE, BM_CPY, 0);
}
```

## Why printing starts silent

YOS itself does not own a text terminal. Alto will eventually provide console
windows, so libc must not scribble directly on the shared display. These all
produce no visible output by default:

```c
putchar('A');
puts("hello");
printf("value=%u\n", value);
write(STDOUT_FILENO, "ok\n", 3);
```

They still report successful output. Applications that need a console must
query a console service explicitly; the YOS ABI does not manufacture a libc
adapter for one.

`getchar()` returns `EOF`, and `trygetchar()` returns zero. For current raw
input use `yos->read_key()`; a later console service can translate editing,
line buffering, and character encoding.

## Publishing a service

A service interface is ordinary process-resident data, usually a struct of
function pointers:

```c
typedef struct counter_api {
    unsigned (*next)(void);
} counter_api_t;

static unsigned next_value(void)
{
    static unsigned value;
    return ++value;
}

static counter_api_t counter = { next_value };

int main(void)
{
    yos_t *yos = (yos_t *)query_service("yos");
    yos_service_t *registration =
        yos->register_service("counter", &counter);
    if (!registration)
        return 1;
    /* Stay alive while clients use counter. */
    for (;;) { }
}
```

The name must fit the kernel's 15-character service field. The interface and
its functions must remain resident for the entire registration. Call
`unregister_service` before invalidating them. ABI 1 registrations are
process-owned and reclaimed on exit. Library initialization instead stages
library-owned registrations until success; acquire these interfaces using
`load_library`, not merely a borrowed query. See
[Loadable Libraries](../the-book-of-yos/LIBRARIES.md).

Registration, query, and removal are atomic kernel-list operations. Calling
the returned interface is ordinary shared code: protect mutable service state
or expose per-client contexts. A query is only a borrowed pointer, so do not
unregister an ordinary service until every borrower has stopped using it.

## Loading a service library

Use `load_library`, rather than `query_service`, when a process needs to own
the lifetime of disk-resident code:

```c
shelllib_api_t *library = yos->load_library(
    "shelllib.svc", YOS_LIBRARY_SHARED);
if (!library)
    return *yos->process_load_error;
if (library->probe() != SHELLLIB_RESULT)
    return 1;
```

`YOS_LIBRARY_PRIVATE` always creates a new instance. `YOS_LIBRARY_SHARED`
reuses a ready instance with the same full XPRG name and image ABI. Every
successful call adds a reference owned by the calling process; its references
are released when its last thread is reclaimed. There is no explicit unload
call in ABI 1. A pointer returned by `query_service` is borrowed and does not
keep a library resident. See the application-focused
[Loadable Libraries](LOADABLE-LIBRARIES.md) chapter for packaging and
initializer rules.

Next: [Memory, Time, and Concurrency](MEMORY-TIME-AND-CONCURRENCY.md).
