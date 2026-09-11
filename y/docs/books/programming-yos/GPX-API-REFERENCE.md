# GPX API Reference

`gpx` is the optional graphics service currently published by the Spectrum
YOS ROM. Include `<gpx.h>`, query it, and create its display context:

```c
gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
if (!gpx) return 1;
gpx_t *screen = gpx->create(GPXM_DEFAULT);
if (!screen) return 2;
```

The service has 23 calls in the exact order below.

## Core types and constants

`coord` is signed 16-bit; `dim` is unsigned 16-bit. A point is `{x,y}` and a
rectangle is `{x0,y0,x1,y1}`. Passing `NULL` for a clipping rectangle selects
the screen bounds. At most `GPX_MAX_POLY_PTS` (12) points may be passed to a
polygon operation.

`CO_BACK` selects a clear pixel and `CO_FORE` a set pixel. `BM_CPY` writes the
selected value; `BM_XOR` toggles it. Line and outline patterns are one-byte
bit patterns. Filled shapes accept a byte array and its length.

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

Initializes graphics and returns the active context. Spectrum YOS supports
`GPXM_DEFAULT`.

```c
gpx_t *screen = gpx->create(GPXM_DEFAULT);
```

### `void destroy(gpx_t *gpx)`

Releases/deactivates a context. The Spectrum implementation uses a static
context, so this is presently a lightweight lifecycle call.

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

Clears the framebuffer.

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
`GPXSB_CURSOR_STD`, `GPXSB_CURSOR_HOURGLASS`, `GPXSB_CURSOR_CARET`, or
`GPXSB_CURSOR_HAND`.

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
