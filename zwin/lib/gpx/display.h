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

/* current global variable */
extern display_t display;

/* initialize display, pass device specific display info */
extern display_t* display_init(void *display_info);

#endif /* _DISPLAY_H */
