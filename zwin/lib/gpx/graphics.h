/*
 * graphics.h
 *
 * functions that manipulate the graphics_t structure.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.04.2021   tstih
 *
 */
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "yos.h"
#include "gpx.h"

#if __LINUX_SDL2__
#include <SDL2/SDL.h>
extern SDL_Surface *surface;
#endif

/* current global variable */
extern graphics_t screen;

/* initialize display, pass device specific display info */
extern graphics_t* graphics_init();

#endif /* _GRAPHICS_H */
