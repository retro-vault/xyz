# GPX API Reference

`gpx` is the optional graphics service currently published by the Spectrum
YOS ROM. Include `<gpx.h>`, query it, and create its display context:

```c
gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
if (!gpx) return 1;
gpx_t *screen = gpx->create(GPXM_DEFAULT);
if (!screen) return 2;
```

The service has 24 calls. The first 23 retain their v1.1.0 slot offsets;
`draw_box` is appended as slot 24. The sections below group calls by subject.

## Core types and constants

`coord` is signed 16-bit; `dim` is unsigned 16-bit. A point is `{x,y}` and a
rectangle is `{x0,y0,x1,y1}`. Passing `NULL` for a clipping rectangle selects
the screen bounds. At most `GPX_MAX_POLY_PTS` (12) points may be passed to a
polygon operation.

`CO_BACK` selects a clear pixel and `CO_FORE` a set pixel. For patterned
operations, `BM_CPY` paints both pattern values, `BM_OR` paints only pattern
one bits, and `BM_XOR` toggles only pattern one bits. Line and outline
patterns are one-byte bit patterns; the standard values are `GPX_LP_SOLID`,
`GPX_LP_DOTTED`, `GPX_LP_DASHED`, and `GPX_LP_DASHED_SHORT`. Filled shapes
accept a byte array and its length.

`gpx_t` reports `width`, `height`, `pages`, and the current text-background
mode. `GPX_TEXT_BG_OPAQUE` paints the glyph background;
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

Use `BMP_SIG_STRIDE(BMP_ENC_1BPP, stride)` for ordinary 1-bit data. Masked
and tiny encodings are `BMP_ENC_1BPP_MASK` and `BMP_ENC_TINY` (with their
masked variants). A font exposes flags, ASCII range, widths, height, advance,
descent, and encoded glyph data.

## Lifecycle and screen information

### `gpx_t *create(gmode mode)`

Allocates an independent six-byte context, owned by the calling process (or
library initializer), or returns `NULL` on exhaustion. Spectrum YOS supports
`GPXM_DEFAULT`. Creation does not clear the shared screen or reset another
context. Each context initially uses opaque text backgrounds.

```c
gpx_t *screen = gpx->create(GPXM_DEFAULT);
```

### `void destroy(gpx_t *gpx)`

Frees the context; `NULL` is harmless. Process cleanup also reclaims forgotten
contexts. Do not destroy a context while another thread is using it.

```c
gpx->destroy(screen);
```

### `void set_page(uint8_t operation, uint8_t page)`

Selects display and/or write page using `PG_DISPLAY`, `PG_WRITE`, or both.
The 48K Spectrum exposes page 0.

```c
gpx->set_page(PG_DISPLAY | PG_WRITE, 0);
```

### `dim width(void)`

Returns the active display width.

```c
dim pixels_across = gpx->width();
```

### `dim height(void)`

Returns the active display height.

```c
dim pixels_down = gpx->height();
```

### `void clear_screen(void)`

Clears the shared physical framebuffer. It is not a per-context canvas or an
atomic frame transaction; coordinate whole-screen ownership between apps.

```c
gpx->clear_screen();
```

### `void set_text_background(gpx_t *gpx, textbg background)`

Chooses opaque or transparent glyph backgrounds for later text drawing.

```c
gpx->set_text_background(screen, GPX_TEXT_BG_TRANSPARENT);
```

## Pixels, lines, and bitmaps

### `void draw_pixel(gpx_t *gpx, coord x, coord y, color c, bmode mode, const rect_t *clip)`

Draws one pixel when it lies inside the display and optional clip.

```c
gpx->draw_pixel(screen, 128, 96, CO_FORE, BM_CPY, NULL);
```

### `uint8_t draw_line(gpx_t *gpx, coord x0, coord y0, coord x1, coord y1, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws a clipped, patterned line and returns the rotated pattern phase so
connected segments can continue it.

```c
uint8_t phase = gpx->draw_line(screen, 0, 0, 255, 191,
                               CO_FORE, BM_CPY, 0xaa, NULL);
```

### `void draw_bitmap(gpx_t *gpx, coord x, coord y, bmp_t *bitmap, const rect_t *clip)`

Draws an encoded bitmap. A small raw 8×8 1-bpp bitmap can be represented as
bytes and cast because the five-byte header is packed:

```c
static uint8_t icon_bytes[] = {
    BMP_SIG_STRIDE(BMP_ENC_1BPP, 1), 8, 8, 8, 0,
    0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24, 0x24, 0x24
};
gpx->draw_bitmap(screen, 20, 20, (bmp_t *)icon_bytes, NULL);
```

## Sprites

A `sprite_t` contains position, bitmap, caller-provided background storage,
and an optional clip. The background buffer must hold at least
`GPX_SPRITE_BG_SIZE` bytes for the current stock cursor format.

### `void show_sprite(gpx_t *gpx, sprite_t *sprite)`

Saves the covered pixels and draws the sprite.

```c
static uint8_t saved[GPX_SPRITE_BG_SIZE];
sprite_t cursor = {40, 40, gpx->get_stock_bitmap(GPXSB_CURSOR_STD),
                   (bmp_t *)saved, NULL};
gpx->show_sprite(screen, &cursor);
```

### `void hide_sprite(gpx_t *gpx, sprite_t *sprite)`

Restores the pixels saved by the matching `show_sprite`.

```c
gpx->hide_sprite(screen, &cursor);
```

Hide a visible sprite before changing its position or reusing its background
buffer, then show it again.

## Rectangles

### `uint8_t draw_box(gpx_t *gpx, const rect_t *rectangle, uint8_t edges, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws selected edges in top, right, bottom, left order. Combine
`GPX_EDGE_LEFT`, `GPX_EDGE_TOP`, `GPX_EDGE_RIGHT`, and `GPX_EDGE_BOTTOM`, or
use `GPX_EDGE_ALL`. Shared corners are drawn once, making the primitive safe
for XOR, and the returned pattern phase can continue another outline.

```c
rect_t box = {10, 10, 100, 60};
uint8_t phase = gpx->draw_box(screen, &box,
                              GPX_EDGE_TOP | GPX_EDGE_BOTTOM,
                              CO_FORE, BM_CPY, GPX_LP_DASHED, NULL);
```

### `void draw_rectangle(gpx_t *gpx, rect_t *rectangle, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws a patterned outline.

```c
rect_t box = {10, 10, 100, 60};
gpx->draw_rectangle(screen, &box, CO_FORE, BM_CPY, 0xff, NULL);
```

### `void fill_rectangle(gpx_t *gpx, rect_t *rectangle, color c, bmode mode, uint8_t *pattern, uint8_t pattern_length, const rect_t *clip)`

Fills a rectangle by cycling through the supplied pattern rows.

```c
uint8_t hatch[] = {0xaa, 0x55};
gpx->fill_rectangle(screen, &box, CO_FORE, BM_CPY,
                    hatch, sizeof hatch, NULL);
```

## Text and built-in assets

### `coord measure_text(const char *text, const font_t *font)`

Returns the pixel advance of a NUL-terminated string.

```c
const font_t *font = gpx->get_system_font();
coord text_width = gpx->measure_text("YOS", font);
```

### `void draw_text(gpx_t *gpx, coord x, coord y, const char *text, const font_t *font, color c, bmode mode, const rect_t *clip)`

Draws a NUL-terminated string using a font descriptor.

```c
gpx->draw_text(screen, 128 - text_width / 2, 90, "YOS", font,
               CO_FORE, BM_CPY, NULL);
```

### `const font_t *get_system_font(void)`

Returns the proportional system font.

```c
const font_t *system_font = gpx->get_system_font();
```

### `const font_t *get_tiny_font(void)`

Returns the compact built-in font.

```c
const font_t *tiny_font = gpx->get_tiny_font();
```

### `bmp_t *get_stock_bitmap(uint8_t which)`

Returns a built-in cursor bitmap. Select `GPXSB_CURSOR_CLASSIC`,
`GPXSB_CURSOR_STD`, `GPXSB_CURSOR_HOURGLASS`, `GPXSB_CURSOR_CARET`,
`GPXSB_CURSOR_HAND`, or `GPXSB_CURSOR_RESIZE`.

```c
bmp_t *hand = gpx->get_stock_bitmap(GPXSB_CURSOR_HAND);
```

## Circles

### `void draw_circle(gpx_t *gpx, coord x, coord y, coord radius, color c, bmode mode, const rect_t *clip)`

Draws a circle outline.

```c
gpx->draw_circle(screen, 128, 96, 30, CO_FORE, BM_CPY, NULL);
```

### `void fill_circle(gpx_t *gpx, coord x, coord y, coord radius, color c, bmode mode, uint8_t *pattern, uint8_t pattern_length, const rect_t *clip)`

Fills a circle with repeated pattern rows.

```c
uint8_t solid[] = {0xff};
gpx->fill_circle(screen, 128, 96, 20, CO_FORE, BM_CPY,
                 solid, sizeof solid, NULL);
```

## Polygons

### `void draw_polygon(gpx_t *gpx, point_t *points, uint8_t count, color c, bmode mode, uint8_t pattern, const rect_t *clip)`

Draws the closed outline through `count` points.

```c
point_t triangle[] = {{128, 20}, {40, 160}, {216, 160}};
gpx->draw_polygon(screen, triangle, 3, CO_FORE, BM_CPY, 0xff, NULL);
```

### `void fill_polygon(gpx_t *gpx, point_t *points, uint8_t count, color c, bmode mode, uint8_t *pattern, uint8_t pattern_length, const rect_t *clip)`

Fills a polygon of no more than 12 points.

```c
uint8_t dots[] = {0x88, 0x22};
gpx->fill_polygon(screen, triangle, 3, CO_FORE, BM_CPY,
                  dots, sizeof dots, NULL);
```

Keep the points, pattern, bitmap, sprite, background, clip, and font storage
alive until the synchronous call returns. The current GPX functions complete
their drawing before returning; they do not retain ordinary primitive arrays.

## Concurrency contract

Each `create` returns independent process-owned state, so changing the text
background of one context does not affect another app. If multiple threads
share one context, they must coordinate semantic changes such as
`set_text_background` with the drawing calls that depend on them.

The physical screen is shared. GPX protects byte-level framebuffer
read/modify/write operations, bitmap/raster rows, and complete sprite
save/show/hide calls against preemption. That prevents neighboring pixels in
one byte from being lost, but it does not turn a line, text string, compound
shape, or `clear_screen` into an atomic frame transaction. Coordinate
overlapping regions between threads and apps. A sprite's background buffer
must remain private from its `show_sprite` through matching `hide_sprite`.
