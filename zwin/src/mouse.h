/*
 * mouse.h
 *
 * basic mouse functions (just control, no drawing!)
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#ifndef _MOUSE_H
#define _MOUSE_H

#include "yos.h"
#include "gpx.h"
#include "cursors.h"

#if __LINUX_SDL2__
#include <SDL2/SDL.h>
#endif

/* at present, we only support 2 button mouse */
#define	MOUSE_RBUTTON	0x01 /* bit 0 */
#define MOUSE_LBUTTON	0x02 /* bit 1 */

/* mouse info */
typedef struct mouse_info_s {
	rect_t rect;                /* current mouse rectangle */
	byte_t status;              /* mouse buttons and status info */
} mouse_info_t;

/* initialize the mouse */
extern void mouse_init();

/* calibrate to position */
extern void mouse_calibrate(word_t x, word_t y);

/* read current mouse info. */
extern void mouse_scan(mouse_info_t *mouse_info);

#endif /* _MOUSE_H */