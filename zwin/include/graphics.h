/*
 * graphics.h
 * include this and -l graphics if you want to use raw graphics functions
 * without the zwin windows.   
 *
 * tomaz stih tue mar 23 2021
 */
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>

/* display properties */
typedef struct display_s {
    uint16_t width;
    uint16_t height;
} display_t;

extern display_t* display_init();

#endif /* _GRAPHICS_H */