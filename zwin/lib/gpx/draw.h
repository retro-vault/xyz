/*
 *	draw.h
 *	drawing routines.
 *
 *	tomaz stih apr 8 2021
 */
#ifndef _LINE_H
#define _LINE_H

#include "yos.h"
#include "display.h"
#include "glyph.h"

#if __LINUX_SDL2__
#include <SDL2/SDL.h> 
#endif

/* line mode can be XOR or COPY */
#define MODE_XOR    1
#define MODE_SET    2
#define MODE_CLR    3

/* special line type (horizontal) */
extern void draw_hline(display_t* d, coord_t y, coord_t x0, coord_t x1, byte_t mode, byte_t pattern);

/* special line type (vertical) */
extern void draw_vline(display_t* d, coord_t x, coord_t y0, coord_t y1, byte_t mode, byte_t pattern);

/* draw individual pixel */
extern void draw_pixel(display_t* d, coord_t x0, coord_t y0, byte_t mode);

/* draw line */
extern void draw_line(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draw circle with radius */
extern void draw_circle(display_t *d, coord_t x0, coord_t y0, coord_t radius, byte_t mode, byte_t pattern);

/* draw ellipse with rectangle */
extern void draw_ellipse(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draws a rectangle */
extern void draw_rectangle(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

#endif /* _LINE_H */