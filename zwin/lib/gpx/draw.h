/*
 * draw.h
 *
 * drawing routines.
 * 
 * NOTES:
 *  bresenhamm implementations: https://gist.github.com/bert/1085538
 * 
 * TODO:
 *  handle drawing modes and patterns.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.04.2021   tstih
 *
 */
#ifndef _LINE_H
#define _LINE_H

#include "yos.h"
#include "graphics.h"
#include "glyph.h"

#if __LINUX_SDL2__
#include <SDL2/SDL.h> 
#endif

/* line mode can be XOR or COPY */
#define MODE_XOR    1
#define MODE_SET    2
#define MODE_CLR    3

/* special line type (horizontal) */
extern void draw_hline(graphics_t* d, coord_t y, coord_t x0, coord_t x1, byte_t mode, byte_t pattern);

/* special line type (vertical) */
extern void draw_vline(graphics_t* d, coord_t x, coord_t y0, coord_t y1, byte_t mode, byte_t pattern);

/* draw individual pixel */
extern byte_t draw_pixel(graphics_t* d, coord_t x0, coord_t y0, byte_t mode);

/* draw line */
extern void draw_line(graphics_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draw circle with radius */
extern void draw_circle(graphics_t *d, coord_t x0, coord_t y0, coord_t radius, byte_t mode, byte_t pattern);

/* draw ellipse with rectangle */
extern void draw_ellipse(graphics_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draws a rectangle */
extern void draw_rectangle(graphics_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

#endif /* _LINE_H */