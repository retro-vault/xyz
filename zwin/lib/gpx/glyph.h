/*
 * glyph.h
 *
 * glyph related consts and structures.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.04.2021   tstih
 *
 */
#ifndef _GLYPH_H
#define _GLYPH_H

#include "yos.h"

/* glyph generation */

/* glyph class, bits 2-0 of the generation flag */
#define GCLS_BITMMAP        0
#define GCLS_MOUSE_CURSOR   1
#define GCLS_FONT           2
#define GCLS_ANIMATION      3

/* glyph drawing mode bits 4-3 of the generation flag */
#define GDWM_TINY           0x00
#define GDWM_RASTER         0x08
#define GDWM_LINES          0x10

/* tiny glyph directions bits 2-0 */
#define TDR_RIGHT           0
#define TDR_RIGHT_UP        1
#define TDR_UP              2
#define TDR_LEFT_UP         3
#define TDR_DOWN            4
#define TDR_RIGHT_DOWN      5
#define TDR_LEFT            6
#define TDR_LEFT_DOWN       7

/* tiny move values bits 4-3 */
#define TPV_MASK            0x18        /* bits 4 and 3 */
#define TPV_SET             0x00        /* set pixel */
#define TPV_RESET           0x08        /* reset pixel */
#define TPV_MOVE            0x10        /* just move */


typedef struct gpy_generation_s {
    unsigned int gcls:3;        /* glyph class */
    unsigned int gdwm:2;        /* drawing mode */
    unsigned int reserved:3;    /* up to 1 byte */
} gpy_generation_t;

/* each glyph file starts with glyph envelope */
typedef struct gpy_envelope_s {
    gpy_generation_t generation;
    byte_t reserved;
    word_t width;
    word_t height;
    byte_t glyphs;              /* number of glyphs following */
} gpy_envelope_t;

/* animation glyph header */
typedef struct gpy_ani_frame_s {
    byte_t index;               /* frame index, 0 based */
    byte_t delay;               /* delay before "playing" */               
} gpy_ani_frame_t;            

/* line structure */
typedef struct gpy_line_s {
    word_t x0;
    word_t y0;
    word_t x1;
    word_t y1;
} gpy_line_t;

/* lines glyph */
typedef struct gpy_lines_s {
    byte_t lcount;              /* number of lines */
    gpy_line_t lines[];         /* array of lines */
} gpy_lines_t;

/* tiny glyph*/
typedef struct gpy_tiny_glyph_s {
    byte_t originx;             /* x origin */
    byte_t originy;             /* y origin */
    byte_t mcount;              /* number of moves */
    byte_t moves[];             /* followed by moves */
} gpy_tiny_glyph_t;   

#endif /* _GLYPH_H */