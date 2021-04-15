/*
 * zmain-sdl.c
 *
 * SDL specific parts of zmain
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "gpx.h"
#include "zmain-sdl.h"

SDL_Window *w;
SDL_Surface *w_surface = NULL;

boolean_t zmain_sdl_start() {

    /* assume error */
    boolean_t success = FALSE;

    /* initialize sdl */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("sdl could not initialize: %s\n", SDL_GetError());
    else
    {
        /* hide mouse cursor, we'll draw our own */
        SDL_ShowCursor(SDL_DISABLE);
        w = SDL_CreateWindow("zwin simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN);
        if (w == NULL)
            printf("window could not be created: %s\n", SDL_GetError());
        else
        {
            SDL_SetWindowSize(w, SCREEN_WIDTH, SCREEN_HEIGHT);
            SDL_ShowWindow(w);
            /* create drawing surface */
            w_surface = SDL_GetWindowSurface(w);
            /* reset the error */
            success = TRUE;
        }
    }
    
    return success;
}

void zmain_sdl_end() {
    /* free the internal surface (created by graphics_init() */
    SDL_FreeSurface(surface);

    SDL_DestroyWindow(w);
    SDL_Quit();
}

boolean_t zmain_sdl_loop() {

    /* first blit surface to the screen */
    SDL_Rect r;
    r.x = 0; r.y = 0; r.w = SCREEN_WIDTH; r.h = SCREEN_HEIGHT;
    SDL_BlitSurface(surface, &r, w_surface, &r);
    SDL_UpdateWindowSurface(w);

    boolean_t success=TRUE;
    SDL_Event e;
    /* clean the sdl event queue */
    while (SDL_PollEvent(&e) != 0)
        /* User requests quit */
        if (e.type == SDL_QUIT)
            success = FALSE;
    /* return quit status */
    return success;
}