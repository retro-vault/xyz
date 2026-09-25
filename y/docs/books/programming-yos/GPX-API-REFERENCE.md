# GPX API Reference

GPX is the graphics section appended to the single `yos_t` interface in
`yos.h`. Query YOS and create a display context:

```c
yos_t *yos = (yos_t *)query_service("yos");
if (!yos) return 1;
gpx_t *screen = yos->gpx_create(GPXM_DEFAULT);
if (!screen) return 2;
```

The graphics section has 24 calls. The first 23 keep their v1.1.0 slot
offsets; `draw_box` was appended later as slot 24. The sections below
group calls by subject.

## Core types and constants

`coord` is signed 16-bit; `dim` is unsigned 16-bit. A point is `{x,y}`
and a rectangle is `{x0,y0,x1,y1}`. Passing `NULL` for a clipping
rectangle selects the screen bounds.

`CO_BACK` selects a clear pixel and `CO_FORE` a set pixel. For patterned
operations, `BM_CPY` paints both pattern values, `BM_OR` paints only
pattern-one bits, and `BM_XOR` toggles only pattern-one bits. Line and
outline patterns are one-byte bit patterns; the standard values are
`GPX_LP_SOLID`, `GPX_LP_DOTTED`, `GPX_LP_DASHED`, and
`GPX_LP_DASHED_SHORT`. Filled shapes accept a byte array and its length.

`gpx_t` reports `width`, `height`, `pages`, and the current
text-background mode. `GPX_TEXT_BG_OPAQUE` paints the glyph background;
`GPX_TEXT_BG_TRANSPARENT` leaves it untouched.

A bitmap begins with:

```c
typedef struct bmp_s {
    uint8_t signature; /* encoding in high nibble, stride-1 in low nibble */
    uint8_t w;
    uint8_t h;
    uint16_t size;
    uint8_t bitmap[];
} bmp_t;
```

Use `BMP_SIG_STRIDE(BMP_ENC_1BPP, stride)` for ordinary 1-bit data.
Masked and tiny encodings are `BMP_ENC_1BPP_MASK` and `BMP_ENC_TINY`
(with their own masked variants). A font descriptor exposes flags, ASCII
range, widths, height, advance, descent, and encoded glyph data.

## Lifecycle and screen information

### `gpx_t *gpx_create(gmode mode)`

Allocates an independent six-byte context, owned by the calling process
(or the library initializer), or returns `NULL` on exhaustion. Spectrum
YOS supports `GPXM_DEFAULT`. Creation neither clears the shared screen
nor resets another context; every new context starts out with an opaque
text background.

```c
gpx_t *screen = yos->gpx_create(GPXM_DEFAULT);
```

### `void gpx_destroy(gpx_t *gpx)`

Frees the context; `NULL` is harmless. Process cleanup also reclaims any
context an application forgot to free. Never destroy a context while
another thread is still using it.

```c
yos->gpx_destroy(screen);
```

### Zero-allocation contexts

`gpx_t` is only six bytes (`width`, `height`, `pages`,
`text_background`, all shown above), and every drawing call only reads
its fields — none of them retain the pointer. A program that draws once
and exits, or that would rather not touch the heap at all, can fill a
stack-local `gpx_t` instead of calling `gpx_create`/`gpx_destroy`:

```c
gpx_t screen;
screen.width = yos->gpx_width();
screen.height = yos->gpx_height();
screen.pages = 1;
screen.text_background = GPX_TEXT_BG_OPAQUE;

yos->gpx_draw_text(&screen, 8, 8, "Hello", font, CO_FORE, BM_CPY, NULL);
```

This is exactly what the shipped `shell.c`/`shell.s` samples do — it
avoids touching the heap in a program that has no other reason to
allocate. Passing a `NULL` context pointer is also legal on every
drawing call: it is treated as an opaque-background context the same
size as the screen, so a one-off draw can skip the local struct
entirely:

```c
yos->gpx_draw_text(NULL, 8, 8, "Hello", font, CO_FORE, BM_CPY, NULL);
```

Prefer `gpx_create`/`gpx_destroy` when a context outlives one function,
is shared by several drawing calls that must agree on a non-default
`text_background`, or should have its lifetime tracked and reclaimed by
process cleanup automatically. Prefer a stack-local or `NULL` context for
a short-lived program, or for a single draw where allocation would be
pure overhead.

### `void gpx_set_page(uint8_t operation, uint8_t page)`

Selects the display and/or write page using `PG_DISPLAY`, `PG_WRITE`, or
both together. The 48K Spectrum exposes only page 0.

```c
yos->gpx_set_page(PG_DISPLAY | PG_WRITE, 0);
```

### `dim gpx_width(void)`

Returns the active display width.

```c
dim pixels_across = yos->gpx_width();
```

### `dim gpx_height(void)`

Returns the active display height.

```c
dim pixels_down = yos->gpx_height();
```

### `void gpx_clear_screen(void)`

Clears the shared physical framebuffer. It is neither a per-context
canvas nor an atomic frame transaction, so coordinate whole-screen
ownership between apps yourself.

```c
yos->gpx_clear_screen();
```

### `void gpx_set_text_background(gpx_t *gpx, textbg background)`

Chooses opaque or transparent glyph backgrounds for later text drawing.

```c
yos->gpx_set_text_background(screen, GPX_TEXT_BG_TRANSPARENT);
```

## Pixels, lines, and bitmaps

### `void gpx_draw_pixel(gpx_t *gpx, coord x, coord y, color c, bmode mode, const rect_t *clip)`

Draws one pixel, when it lies inside the display and any optional clip.

```c
yos->gpx_draw_pixel(screen, 128, 96, CO_FORE, BM_CPY, NULL);
```

### `uint8_t gpx_draw_line(gpx_t *gpx, coord x0, coord y0, coord x1, coord y1, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws a clipped, patterned line and returns the rotated pattern phase,
so connected segments can continue it seamlessly.

```c
uint8_t phase = yos->gpx_draw_line(screen, 0, 0, 255, 191,
                               CO_FORE, BM_CPY, 0xaa, NULL);
```

### `void gpx_draw_bitmap(gpx_t *gpx, coord x, coord y, bmp_t *bitmap, const rect_t *clip)`

Draws an encoded bitmap. A small raw 8×8 1-bpp bitmap can be represented
as plain bytes and cast directly, since the five-byte header is packed:

```c
static uint8_t icon_bytes[] = {
    BMP_SIG_STRIDE(BMP_ENC_1BPP, 1), 8, 8, 8, 0,
    0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24, 0x24, 0x24
};
yos->gpx_draw_bitmap(screen, 20, 20, (bmp_t *)icon_bytes, NULL);
```

## Sprites

A `sprite_t` contains position, bitmap, caller-provided background
storage, and an optional clip. The background buffer must hold at least
`GPX_SPRITE_BG_SIZE` bytes for the current stock cursor format.

### `void gpx_show_sprite(gpx_t *gpx, sprite_t *sprite)`

Saves the covered pixels and draws the sprite.

```c
static uint8_t saved[GPX_SPRITE_BG_SIZE];
sprite_t cursor = {40, 40, yos->gpx_get_stock_bitmap(GPXSB_CURSOR_STD),
                   (bmp_t *)saved, NULL};
yos->gpx_show_sprite(screen, &cursor);
```

### `void gpx_hide_sprite(gpx_t *gpx, sprite_t *sprite)`

Restores the pixels saved by the matching `show_sprite`.

```c
yos->gpx_hide_sprite(screen, &cursor);
```

Hide a visible sprite before changing its position or reusing its
background buffer, then show it again.

## Rectangles

### `uint8_t gpx_draw_box(gpx_t *gpx, const rect_t *rectangle, uint8_t edges, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws selected edges in top, right, bottom, left order. Combine
`GPX_EDGE_LEFT`, `GPX_EDGE_TOP`, `GPX_EDGE_RIGHT`, and
`GPX_EDGE_BOTTOM`, or just use `GPX_EDGE_ALL`. Shared corners are drawn
once, which keeps the primitive safe under XOR, and the returned pattern
phase can continue another outline.

```c
rect_t box = {10, 10, 100, 60};
uint8_t phase = yos->gpx_draw_box(screen, &box,
                              GPX_EDGE_TOP | GPX_EDGE_BOTTOM,
                              CO_FORE, BM_CPY, GPX_LP_DASHED, NULL);
```

### `void gpx_draw_rectangle(gpx_t *gpx, rect_t *rectangle, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws a patterned outline.

```c
rect_t box = {10, 10, 100, 60};
yos->gpx_draw_rectangle(screen, &box, CO_FORE, BM_CPY, 0xff, NULL);
```

### `void gpx_fill_rectangle(gpx_t *gpx, rect_t *rectangle, color c, bmode mode, uint8_t *pattern, uint8_t pattern_length, const rect_t *clip)`

Fills a rectangle by cycling through the supplied pattern rows.

```c
uint8_t hatch[] = {0xaa, 0x55};
yos->gpx_fill_rectangle(screen, &box, CO_FORE, BM_CPY,
                    hatch, sizeof hatch, NULL);
```

## Text and built-in assets

### `coord gpx_measure_text(const char *text, const font_t *font)`

Returns the pixel advance of a NUL-terminated string.

```c
const font_t *font = yos->gpx_get_system_font();
coord text_width = yos->gpx_measure_text("YOS", font);
```

### `void gpx_draw_text(gpx_t *gpx, coord x, coord y, const char *text, const font_t *font, color c, bmode mode, const rect_t *clip)`

Draws a NUL-terminated string using a font descriptor.

```c
yos->gpx_draw_text(screen, 128 - text_width / 2, 90, "YOS", font,
               CO_FORE, BM_CPY, NULL);
```

### `const font_t *gpx_get_system_font(void)`

Returns the proportional system font.

```c
const font_t *system_font = yos->gpx_get_system_font();
```

### `const font_t *gpx_get_tiny_font(void)`

Returns the compact built-in font.

```c
const font_t *tiny_font = yos->gpx_get_tiny_font();
```

### `bmp_t *gpx_get_stock_bitmap(uint8_t which)`

Returns a built-in cursor bitmap. Select `GPXSB_CURSOR_CLASSIC`,
`GPXSB_CURSOR_STD`, `GPXSB_CURSOR_HOURGLASS`, `GPXSB_CURSOR_CARET`,
`GPXSB_CURSOR_HAND`, or `GPXSB_CURSOR_RESIZE`.

```c
bmp_t *hand = yos->gpx_get_stock_bitmap(GPXSB_CURSOR_HAND);
```

## Circles

### `void gpx_draw_circle(gpx_t *gpx, coord x, coord y, coord radius, color c, bmode mode, const rect_t *clip)`

Draws a circle outline.

```c
yos->gpx_draw_circle(screen, 128, 96, 30, CO_FORE, BM_CPY, NULL);
```

### `void gpx_fill_circle(gpx_t *gpx, coord x, coord y, coord radius, color c, bmode mode, uint8_t *pattern, uint8_t pattern_length, const rect_t *clip)`

Fills a circle with repeated pattern rows.

```c
uint8_t solid[] = {0xff};
yos->gpx_fill_circle(screen, 128, 96, 20, CO_FORE, BM_CPY,
                 solid, sizeof solid, NULL);
```

Keep pattern, bitmap, sprite, background, clip, and font
storage alive until the synchronous call returns. GPX functions complete
their drawing before returning, so they never retain an ordinary
primitive array beyond the call itself.

## A complete program

The snippets above each show one call at a time. This program uses the
full `gpx_create`/`gpx_destroy` lifecycle — from a cold `query_service`
all the way to a clean exit — to draw a titled, outlined panel with a
filled circle and centred text:

```c
#include <yos.h>

void main(void)
{
    static const char title[] = "YOS Graphics";
    yos_t *yos = (yos_t *)query_service("yos");
    gpx_t *screen;
    const font_t *font;
    rect_t panel = {20, 20, 235, 171};
    coord title_width;
    uint8_t hatch[] = {0xaa, 0x55};

    if (!yos || yos->version() < YOS_VERSION)
        for (;;) {}

    screen = yos->gpx_create(GPXM_DEFAULT);
    if (!screen)
        for (;;) {}

    font = yos->gpx_get_system_font();
    title_width = yos->gpx_measure_text(title, font);

    yos->gpx_clear_screen();
    yos->gpx_draw_rectangle(screen, &panel, CO_FORE, BM_CPY,
                             GPX_LP_SOLID, NULL);
    yos->gpx_fill_circle(screen, 128, 110, 30, CO_FORE, BM_CPY,
                          hatch, sizeof hatch, NULL);
    yos->gpx_set_text_background(screen, GPX_TEXT_BG_OPAQUE);
    yos->gpx_draw_text(screen, (coord)((256 - title_width) / 2), 30,
                        title, font, CO_FORE, BM_CPY, NULL);

    yos->gpx_destroy(screen);

    for (;;) {}
}
```

Build and package it exactly as in
[Your First Process](YOUR-FIRST-PROCESS.md):

```sh
mkdir -p build/examples/yos bin/y/arch/48
bin/x/bin/xcc -Os --platform=yos gfxdemo.c -o build/examples/yos/gfxdemo.xl
bin/x/bin/xprog --process --name gfxdemo --stack-size 512 --min-os 1 \
  build/examples/yos/gfxdemo.xl -o bin/y/arch/48/gfxdemo.prc
```

Then boot it the same way as any other process — see
[Running your program](YOUR-FIRST-PROCESS.md#running-your-program).

## Concurrency contract

Each `create` returns independent, process-owned state, so changing the
text background of one context never affects another app's. If multiple
threads share one context, they must coordinate semantic changes such as
`set_text_background` with the drawing calls that depend on them.

The physical screen, however, is shared. GPX protects byte-level
framebuffer read/modify/write operations, bitmap/raster rows, and
complete sprite save/show/hide calls against preemption — enough to stop
neighboring pixels in one byte from being lost, but not enough to turn a
line, a text string, a compound shape, or `clear_screen` into an atomic
frame transaction. Coordinate overlapping regions between threads and
apps yourself. A sprite's background buffer must stay private from its
`show_sprite` call through the matching `hide_sprite`.
