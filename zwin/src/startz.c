/*
 *	startz.h
 *	functions for working with the display.
 *
 *	tomaz stih apr 8 2021
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "gpx.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

typedef struct glyph_header_s {
    byte_t generation;
    byte_t reserved;
    word_t width;
    word_t height;
    byte_t frames;
} glyph_header_t;

typedef struct frame_header_s {
    byte_t index;
    byte_t delay;
    word_t lines;
} frame_header_t;

typedef struct line_s {
    word_t x0;
    word_t y0;
    word_t x1;
    word_t y1;
} line_t;

extern byte_t charlie[];

void animate(display_t *d) {

    glyph_header_t* gh=(glyph_header_t*)&charlie;
    frame_header_t* fh=(frame_header_t*)&(charlie[7]);

    int i;
    line_t *pline=(line_t *)&(charlie[11]);
    for(i=0;i<fh->lines;i++) {
        draw_line(d,pline->x0,pline->y0,pline->x1,pline->y1,0,0xaa);
        pline++;
    }
}

int main(int argc, char *args[])
{
    /* The window we'll be rendering to */
    SDL_Window *window = NULL;

    /* The surface contained by the window */
    SDL_Surface *wgpx = NULL;

    /* drawing surface */
    SDL_Surface *g;

    display_t *d;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    else
    {
        g = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

        d = display_init((void *)g);

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

        animate(d);

        SDL_Rect r;
        r.x = 0;
        r.y = 0;
        r.w = SCREEN_WIDTH;
        r.h = SCREEN_HEIGHT;

        SDL_BlitSurface(g, &r, wgpx, &r);

        /* Update the surface */
        SDL_UpdateWindowSurface(window);
    }

    SDL_FreeSurface(g);

    /* Destroy window */
    SDL_DestroyWindow(window);

    /*
	SDL_FreeCursor(cursor);
	*/

    /* Quit SDL subsystems */
    SDL_Quit();

    return 0;
}
