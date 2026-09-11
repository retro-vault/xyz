# Files, Input, and Graphics

## POSIX-style files

Use the standard headers. The YOS platform wrappers call the identically named
entries in `yos_t` and copy the kernel error cell to libc `errno` on failure:

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

Supported flags are `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`,
`O_APPEND`, and `O_EXCL`. `O_BINARY` and `O_TEXT` are zero: no newline or
end-of-file translation occurs. `lseek` accepts `SEEK_SET`, `SEEK_CUR`, and
`SEEK_END` with a signed 32-bit `off_t`.

Descriptors 0, 1, and 2 are the synthetic console. Reading 0 returns end of
file. Writes to 1 or 2 feed the optional character hook and otherwise remain
silent. Other descriptors belong to the YOS/esxDOS filesystem.

## Paths, metadata, and directories

The backing filesystem uses short 8.3 names. Keep components to eight base
characters plus a three-character extension.

```c
#include <dirent.h>
#include <sys/stat.h>

DIR *dir = opendir(".");
if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* entry->d_name, d_size, d_type, d_attributes */
    }
    closedir(dir);
}
```

`d_type` is `DT_REG` or `DT_DIR`; the native esxDOS attributes remain in
`d_attributes`. Directory entries are owned by the directory stream and are
reused by subsequent reads. `rewinddir` returns to the first entry.

`stat` and `fstat` fill `st_mode` and 32-bit `st_size`; use `S_ISREG` and
`S_ISDIR`. `mkdir` accepts a mode for source compatibility, but esxDOS does
not implement Unix permissions.

Enumerate physical block devices safely into caller storage:

```c
yos_disk_info_t disks[4];
int count = enumerate_disks(disks, 4);
```

Each record has the esxDOS device byte, flags, and a 32-bit block count.

## Keyboard

The keyboard call is nonblocking and returns queued transitions:

```c
uint8_t event = yos->read_key();
if (event) {
    uint8_t key = event & YOS_KEY_CODE;       /* one-based raw matrix code */
    int down = (event & YOS_KEY_DOWN) != 0;
}
```

Zero means that the queue is empty. YOS scans the Spectrum matrix at 50 Hz and
queues both press and release transitions. This is a physical-key interface,
not ASCII input.

## Kempston mouse

Calibrate once, then poll:

```c
yos_mouse_state_t mouse;
yos->calibrate_mouse(128, 96);
yos->read_mouse(&mouse);
```

`x` and `y` are the accumulated cursor position. `buttons` is the current
button state and `changed_buttons` identifies buttons that changed since the
previous poll.

## Graphics as an optional service

```c
#include <gpx.h>

gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
if (!gpx)
    return 1;
gpx_t *screen = gpx->create(GPXM_DEFAULT);
if (!screen)
    return 2;

gpx->clear_screen();
gpx->draw_line(screen, 0, 0, 255, 191,
               CO_FORE, BM_CPY, 0xff, NULL);
```

Coordinates are signed, allowing primitives to be clipped at screen edges.
`CO_BACK` clears pixels and `CO_FORE` sets them. `BM_CPY` copies the selected
colour; `BM_XOR` toggles. Many calls accept an optional clipping rectangle;
pass `NULL` for the whole screen.

Text uses bitmap font descriptors rather than libc console output:

```c
const font_t *font = gpx->get_system_font();
gpx->set_text_background(screen, GPX_TEXT_BG_TRANSPARENT);
gpx->draw_text(screen, 8, 8, "Hello", font,
               CO_FORE, BM_CPY, NULL);
```

Sprites save their old background in a caller-supplied bitmap. Polygons accept
at most `GPX_MAX_POLY_PTS` (12) points. See the
[complete GPX reference](GPX-API-REFERENCE.md) for bitmap layouts, patterns,
and examples of every primitive.
