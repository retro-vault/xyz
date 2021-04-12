/*
 *	display.h
 *	functions for working with the display.
 *
 *	tomaz stih apr 8 2021
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "yos.h"
#include "rect.h"

#if __LINUX_SDL2__
#define XMAX    639
#define YMAX    399
#elif __ID_PARTNER__
#define XMAX    1023
#define YMAX    511
#elif __ZX_SPECTRUM__
#define XMAX    255
#define YMAX    191
#endif

/* current global variable */
extern display_t display;

/* initialize display, pass device specific display info */
extern display_t* display_init(void *display_info);

#endif /* _DISPLAY_H */
