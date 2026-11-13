/* Public graphics types used by the YOS service table on every target. */
#ifndef YOS_GPX_H
#define YOS_GPX_H
#include <stdint.h>
#ifndef __LIBGPX_H__
/* libgpx public formats and constants, from upstream commit 0ef6f070.
 * GPL-2.0, Copyright (C) 2026 tomaz stih. */
/* Signed screen coordinate and unsigned screen dimension. */
typedef int16_t coord;
typedef uint16_t dim;

#define CO_BACK 0x00
#define CO_FORE 0x01
typedef uint8_t color;

#define BM_CPY 0x00
#define BM_XOR 0x01
#define BM_OR  0x02
typedef uint8_t bmode;

#define GPX_LP_SOLID        0xff
#define GPX_LP_DOTTED       0xaa
#define GPX_LP_DASHED       0xf0
#define GPX_LP_DASHED_SHORT 0xcc

#define GPX_TEXT_BG_OPAQUE      0x00
#define GPX_TEXT_BG_TRANSPARENT 0x01
typedef uint8_t textbg;

typedef struct rect_s {
    coord x0;
    coord y0;
    coord x1;
    coord y1;
} rect_t;

#define BMP_ENC_1BPP      0x0
#define BMP_ENC_1BPP_MASK 0x1
#define BMP_ENC_TINY      0x2
#define BMP_ENC_TINY_MASK 0x3
#define BMP_SIG(enc) ((uint8_t)(((enc) & 0x0f) << 4))
#define BMP_ENC(sig) ((uint8_t)(((sig) >> 4) & 0x0f))
#define BMP_STRIDE_ENC(stride) ((uint8_t)(((stride) - 1) & 0x0f))
#define BMP_STRIDE(sig) ((uint8_t)(((sig) & 0x0f) + 1))
#define BMP_SIG_STRIDE(enc, stride) \
    ((uint8_t)(BMP_SIG(enc) | BMP_STRIDE_ENC(stride)))
#define S_BMP BMP_SIG(BMP_ENC_1BPP)

#if defined(__GNUC__) && !defined(__XCC_MODEL__)
#pragma pack(push, 1)
#endif
typedef struct bmp_s {
    uint8_t signature;
    uint8_t w;
    uint8_t h;
    uint16_t size;
    uint8_t bitmap[];
} bmp_t;
#if defined(__GNUC__) && !defined(__XCC_MODEL__)
#pragma pack(pop)
#endif

typedef struct font_s {
    uint8_t flags;
    uint8_t first_ascii;
    uint8_t last_ascii;
    uint8_t empty_width;
    uint8_t max_glyph_width;
    uint8_t glyph_height;
    uint8_t advance;
    uint8_t descent;
    uint8_t data[];
} font_t;

#define FONT_FLAG_PROPORTIONAL 0x01
#define FONT_FLAG_OFFSETS_BE   0x02
#define FONT_FLAG_VECTOR       0x04

typedef struct gpx_s {
    dim width;
    dim height;
    uint8_t pages;
    textbg text_background;
} gpx_t;

#define GPX_SPRITE_BG_HEADER_SIZE  5
#define GPX_SPRITE_BG_PAYLOAD_SIZE 32
#define GPX_SPRITE_BG_SIZE \
    (GPX_SPRITE_BG_HEADER_SIZE + GPX_SPRITE_BG_PAYLOAD_SIZE)

typedef struct sprite_s {
    coord x;
    coord y;
    bmp_t *bitmap;
    bmp_t *background;
    const rect_t *clip;
} sprite_t;

#define GPXM_DEFAULT        0
#define GPXM_CPC_640X200    0
#define GPXM_CPC_320X200    1
#define GPXM_RASTA_1920X1080 0
typedef uint8_t gmode;

#define PG_DISPLAY 0x01
#define PG_WRITE   0x02

#define GPXSB_CURSOR_CLASSIC   0
#define GPXSB_CURSOR_STD       1
#define GPXSB_CURSOR_HOURGLASS 2
#define GPXSB_CURSOR_CARET     3
#define GPXSB_CURSOR_HAND      4
#define GPXSB_CURSOR_RESIZE    5

#define GPX_EDGE_LEFT   0x01
#define GPX_EDGE_TOP    0x02
#define GPX_EDGE_RIGHT  0x04
#define GPX_EDGE_BOTTOM 0x08
#define GPX_EDGE_ALL    0x0f

#endif
#endif
