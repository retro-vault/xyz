/*
 *	zmain-sdl.c
 *	z windows main() function (for the SDL).
 *
 *	tomaz stih apr 8 2021
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "gpx.h"

int main(int argc, char *args[])
{
    /* The window we'll be rendering to */
    SDL_Window *window = NULL;

    /* The surface contained by the window */
    SDL_Surface *wgpx = NULL;

    /* drawing surface */
    SDL_Surface *g;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    else
    {
        g = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

        SDL_ShowCursor(SDL_DISABLE);
        /* SDL_WarpMouse( x, y ); */

        window = SDL_CreateWindow("zwin simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN);
        if (window == NULL)
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        else
        {
            SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

            SDL_ShowWindow(window);

            /*Get window surface */
            wgpx = SDL_GetWindowSurface(window);

            /*Update the surface */
            SDL_UpdateWindowSurface(window);
        }
    }

    /* Main loop flag */
    bool quit = false;

    /* Event handler */
    SDL_Event e;

    /*While application is running */
    while (!quit)
    {
        /* Handle events on queue */
        while (SDL_PollEvent(&e) != 0)
        {
            /* User requests quit */
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        /* Blit to the surface. */
        SDL_Rect r;
        r.x = 0; r.y = 0; r.w = SCREEN_WIDTH; r.h = SCREEN_HEIGHT;
        SDL_BlitSurface(g, &r, wgpx, &r);

        /* Update the surface */
        SDL_UpdateWindowSurface(window);
    }

    /* Free the surface */
    SDL_FreeSurface(g);

    /* Destroy window */
    SDL_DestroyWindow(window);

    /* Quit SDL subsystems */
    SDL_Quit();

    return 0;
}
