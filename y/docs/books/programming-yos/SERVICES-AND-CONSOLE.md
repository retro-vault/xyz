# Services and Console Output

YOS uses named tables of function pointers instead of a fixed list of
call numbers. That choice is what lets facilities stay optional and
version independently of one another.

## The one primitive: `query_service`

```c
#include <yos.h>

yos_t *yos = (yos_t *)query_service("yos");
```

The function executes `RST 0x18` with the service name and returns the
table, or `NULL` when no service has that name. The comparison is
case-sensitive. Always test an optional service before dereferencing it.

`YOS_VERSION` is the ABI required by the installed header — currently 1.
XPRG's `--min-os 1` is the loader-side check. An application can
additionally inspect the returned table's `version()` for diagnostics or
unusual launchers.

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

YOS itself does not own a text terminal. Alto will eventually provide
console windows, so libc must not scribble directly on the shared
display in the meantime. All of the following produce no visible output
by default:

```c
putchar('A');
puts("hello");
printf("value=%u\n", value);
write(STDOUT_FILENO, "ok\n", 3);
```

They still report successful output. Applications that need a console
must query a console service explicitly — the YOS ABI does not
manufacture a libc adapter for one on its own.

`getchar()` returns `EOF`, and `trygetchar()` returns zero. For raw input
today, use `yos->read_key()`; a later console service will be able to
translate editing, line buffering, and character encoding on top of it.

## Publishing a service

A service interface is ordinary process-resident data, usually a struct
of function pointers:

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

The name must fit the kernel's 15-character service field. The interface
and its functions must remain resident for the entire registration. Call
`unregister_service` before invalidating them. ABI 1 registrations are
process-owned and get reclaimed on exit. Library initialization instead
stages library-owned registrations until it succeeds; acquire these
interfaces through `load_library`, not a merely borrowed query. See
[Loadable Libraries](../the-book-of-yos/LIBRARIES.md).

Registration, query, and removal are all atomic kernel-list operations.
Calling the returned interface is ordinary shared code, though, so you
still need to protect mutable service state or expose per-client
contexts yourself. A query only ever returns a borrowed pointer, so never
unregister an ordinary service until every borrower has stopped using
it.

## Loading a service library

Use `load_library` rather than `query_service` when a process needs to
own the lifetime of disk-resident code:

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
successful call adds a reference owned by the calling process, and those
references are released once its last thread is reclaimed. There is no
explicit unload call in ABI 1. A pointer returned by `query_service` is
merely borrowed and does not keep a library resident on its own. See the
application-focused [Loadable Libraries](LOADABLE-LIBRARIES.md) chapter
for packaging and initializer rules.

Next: [Memory, Time, and Concurrency](MEMORY-TIME-AND-CONCURRENCY.md).
