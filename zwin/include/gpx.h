/*
 * gpx.h
 * include this and -l gpx if you want to use raw graphics functions
 * without the zwin windows. 
 *
 * tomaz stih tue mar 23 2021
 */
#ifndef _GPX_H
#define _GPX_H

#include "yos.h"

/* gpx types */
typedef int         coord_t;

/* basic structures */

/* the rectangle */
typedef struct rect_s {
	coord_t x0;
	coord_t y0;
	coord_t x1;
	coord_t y1;
} rect_t;

/* the display */
typedef struct display_s {
    coord_t xmin;
    coord_t ymin;
    coord_t xmax;
    coord_t ymax;
    void *display_info;
} display_t;

extern display_t* display_init(void *display_info);


/* basic drawing functions */

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

#endif /* _GPX_H */