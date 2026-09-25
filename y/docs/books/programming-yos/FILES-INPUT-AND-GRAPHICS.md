# Files, Input, and Graphics

## POSIX-style files

Use the standard headers. The YOS platform wrappers call the identically
named entries in `yos_t` and copy the kernel error cell into libc
`errno` on failure:

```c
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int fd = open("NOTES.TXT", O_RDWR | O_CREAT | O_TRUNC);
if (fd >= 0) {
    static const char text[] = "YOS\n";
    write(fd, text, sizeof text - 1);
    fsync(fd);
    close(fd);
}
```

Supported flags are `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`,
`O_TRUNC`, `O_APPEND`, and `O_EXCL`. `O_BINARY` and `O_TEXT` are zero —
no newline or end-of-file translation happens either way. `lseek`
accepts `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`, with a signed 32-bit
`off_t`.

Descriptors 0, 1, and 2 are the synthetic console. Reading from 0 returns
end of file. Writes to 1 or 2 stay silent. Every other descriptor belongs
to the YOS/esxDOS filesystem.

Descriptors and the current directory are system-wide. YOS serializes
each descriptor operation, including an append seek plus write, but a
multi-call sequence such as `chdir` followed by `open` still needs
application-level coordination. Do not close a descriptor, or free a
buffer, while another thread is using it. The raw kernel error cell is
per-thread, but libc `errno` is only process-local — protect a wrapper
call together with its error read, or use the raw table entry and
`*yos->error_number` instead. See
[Concurrency](MEMORY-TIME-AND-CONCURRENCY.md).

## Paths, metadata, and directories

The backing filesystem uses short 8.3 names. Keep components to eight
base characters plus a three-character extension.

```c
#include <sys/stat.h>
#include <yos.h>

yos_directory_t *dir = yos->opendir(".");
if (dir) {
    yos_directory_entry_t *entry;
    while ((entry = yos->readdir(dir)) != NULL) {
        /* entry->d_name, d_size, d_type, d_attributes */
    }
    yos->closedir(dir);
}
```

`d_type` is `YOS_DIRECTORY_TYPE_REG` or `YOS_DIRECTORY_TYPE_DIR`; the
native esxDOS attributes remain in `d_attributes`. Directory entries are
owned by the directory stream and get reused on the next read — copy an
entry before another thread reads the same stream. `yos->rewinddir`
returns to the first entry.

`stat` and `fstat` fill `st_mode` and a 32-bit `st_size`; use `S_ISREG`
and `S_ISDIR` to interpret the mode. `mkdir` accepts a mode argument for
source compatibility, but esxDOS itself does not implement Unix
permissions.

Enumerate physical block devices safely into caller-owned storage:

```c
yos_disk_info_t disks[4];
int count = yos->enumerate_disks(disks, 4);
```

Each record carries the esxDOS device byte, flags, and a 32-bit block
count.

## Keyboard

The keyboard call is nonblocking and returns queued transitions:

```c
uint8_t event = yos->read_key();
if (event) {
    uint8_t key = event & YOS_KEY_CODE;       /* one-based raw matrix code */
    int down = (event & YOS_KEY_DOWN) != 0;
}
```

Zero means the queue is empty. YOS scans the Spectrum matrix at 50 Hz and
queues both press and release transitions — this is a physical-key
interface, not ASCII input.

## Kempston mouse

Calibrate once, then read the latest state:

```c
yos_mouse_state_t mouse;
yos->calibrate_mouse(128, 96);
yos->read_mouse(&mouse);
```

The kernel's timer chain samples the Kempston counters at 50 Hz and
maintains bounded absolute screen coordinates (`x` 0–255, `y` 0–191).
`read_mouse` only snapshots that state — it never touches the ports
directly. `buttons` gives the latest button state, while
`changed_buttons` accumulates every transition since the previous
`read_mouse` call and is consumed by that call, so a short click is never
lost even when an application reads less often than 50 Hz.

## Graphics

GPX types, constants, and calls are all part of the single `yos_t`
interface in `yos.h`.

```c
yos_t *yos = (yos_t *)query_service("yos");
if (!yos)
    return 1;
gpx_t *screen = yos->gpx_create(GPXM_DEFAULT);
if (!screen)
    return 2;

yos->gpx_clear_screen();
yos->gpx_draw_line(screen, 0, 0, 255, 191,
               CO_FORE, BM_CPY, 0xff, NULL);
```

A heap-owned context is not required: a stack-local `gpx_t`, or even a
`NULL` context pointer, both work on every drawing call, and are the
cheaper choice for a program that draws once and exits. See
[Zero-allocation contexts](GPX-API-REFERENCE.md#zero-allocation-contexts)
in the API reference for both patterns.

Coordinates are signed, which lets primitives clip cleanly at the screen
edges. Every `create` returns independent, process-owned state, so
text-background settings on one context never affect another app's.
Colors, modes, patterns, and clips are already explicit call arguments.
The physical framebuffer, though, remains shared: raster critical
sections prevent neighboring pixels within a byte from being lost, but
overlapping artwork and sprite show/hide lifetimes still need
application-level coordination. See
[Concurrency](MEMORY-TIME-AND-CONCURRENCY.md).

`CO_BACK` clears pixels and `CO_FORE` sets them. For patterned
operations, `BM_CPY` paints both pattern values, `BM_OR` preserves
pattern-zero pixels, and `BM_XOR` toggles pattern-one pixels. Many calls
accept an optional clipping rectangle; pass `NULL` for the whole screen.

Text uses bitmap font descriptors rather than libc console output:

```c
const font_t *font = yos->gpx_get_system_font();
yos->gpx_set_text_background(screen, GPX_TEXT_BG_TRANSPARENT);
yos->gpx_draw_text(screen, 8, 8, "Hello", font,
               CO_FORE, BM_CPY, NULL);
```

Sprites save their old background in a caller-supplied bitmap. See the
[complete GPX reference](GPX-API-REFERENCE.md) for bitmap layouts,
patterns, and a worked example of every primitive.

Next: [Loadable Libraries](LOADABLE-LIBRARIES.md).
