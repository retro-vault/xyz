/*
 * zmain-sdl.h
 *
 * SDL specific parts of zmain
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#ifndef _ZMAIN_SDL_H
#define _ZMAIN_SDL_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "gpx.h"

/* internal from graphics.h */
extern SDL_Surface *surface;

/* zmain supplements */
extern boolean_t zmain_sdl_start();
extern void zmain_sdl_end();
extern boolean_t zmain_sdl_loop();

#endif _ZMAIN_SDL_H