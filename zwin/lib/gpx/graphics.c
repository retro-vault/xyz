/*
 * graphics.c
 *
 * functions that manipulate the graphics_t structure.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 08.04.2021   tstih
 *
 */
#include "graphics.h"

#if __LINUX_SDL2__
SDL_Surface *surface;
#endif

graphics_t screen;

graphics_t* graphics_init() {
    
    /* initialize screen area and clipping */
    screen.area.x0=screen.area.y0=screen.clip.x0=screen.clip.x1=0;
    screen.area.x1=screen.clip.x1=SCREEN_WIDTH-1;
    screen.area.y1=screen.clip.y1=SCREEN_HEIGHT-1;
    
    #if __LINUX_SDL2__
    /* custom initialization */
    surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    #endif __LINUX_SDL2__
    
    return &screen;
}