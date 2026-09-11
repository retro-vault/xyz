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

The YOS CRT already performs that query. Prefer the cached spelling for the
kernel table:

```c
yos_t *yos = yos_get_api();
if (!yos || yos->version() < YOS_VERSION)
    return 1;
```

`YOS_VERSION` is the ABI required by the installed header, currently 8. XPRG's
`--min-os 8` is the loader-side check; the runtime check above is useful for
diagnostics and unusual launchers.

Graphics is separate:

```c
#include <gpx.h>

gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
if (gpx) {
    gpx_t *screen = gpx->create(GPXM_DEFAULT);
    if (screen)
        gpx->draw_pixel(screen, 20, 20, CO_FORE, BM_CPY, 0);
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

They still report successful output. A process may install the nonstandard,
process-local character sink from `yos.h`:

```c
#include <stdio.h>
#include <yos.h>

static void alto_character(char ch)
{
    /* Send ch to an Alto console service here. */
}

int main(void)
{
    yos_putchar_hook_t old = yos_set_putchar_hook(alto_character);
    printf("Now every character reaches alto_character.\n");
    yos_set_putchar_hook(old);
    return 0;
}
```

The hook is called once for every character. It must avoid recursively calling
`putchar`, `puts`, `printf`, or `write(1, ...)`. Keep it short or buffer the
characters for another subsystem.

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
    yos_t *yos = yos_get_api();
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
`unregister_service` before invalidating them. ABI 8 registrations made by
this public adapter are kernel-owned, so an orderly process should explicitly
unregister; automatic ownership for this call is not yet exposed.

Next: [Memory, Time, and Concurrency](MEMORY-TIME-AND-CONCURRENCY.md).
