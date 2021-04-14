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

/* consts */
#if __LINUX_SDL2__
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   512
#elif __ID_PARTNER__
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   512
#elif __ZX_SPECTRUM__
#define SCREEN_WIDTH    256
#define SCREEN_HEIGHT   192
#endif


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

/* glyphs */

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
#define TPV_SET             0x00        /* set pixel */
#define TPV_RESET           0x08        /* reset pixel */
#define TPV_MOVE            0x10        /* just move */

extern display_t* display_init(void *display_info);


/* basic drawing functions */

#define DWM_SET             0x00
#define DWM_RESET           0x00

/* special line type (horizontal) */
extern void draw_hline(display_t* d, coord_t y, coord_t x0, coord_t x1, byte_t mode, byte_t pattern);

/* special line type (vertical) */
extern void draw_vline(display_t* d, coord_t x, coord_t y0, coord_t y1, byte_t mode, byte_t pattern);

/* draw individual pixel */
extern byte_t draw_pixel(display_t* d, coord_t x0, coord_t y0, byte_t mode);

/* draw line */
extern void draw_line(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draw circle with radius */
extern void draw_circle(display_t *d, coord_t x0, coord_t y0, coord_t radius, byte_t mode, byte_t pattern);

/* draw ellipse with rectangle */
extern void draw_ellipse(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

/* draws a rectangle */
extern void draw_rectangle(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern);

#endif /* _GPX_H */