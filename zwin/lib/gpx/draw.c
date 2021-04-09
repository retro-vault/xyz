/*
 *	draw.c
 *	drawing routines.
 *
 *  notes:
 *      bresenhamm implementations: https://gist.github.com/bert/1085538
 * 
 *  todo:
 *      handle drawing modes and patterns.
 * 
 *	tomaz stih apr 8 2021
 */
#include "draw.h"

void draw_hline(display_t *d, coord_t y, coord_t x0, coord_t x1, byte_t mode, byte_t pattern)
{

    SDL_Surface *surface = (SDL_Surface *)d->display_info;
    SDL_LockSurface(surface);

    uint8_t *pixels = (uint8_t *)surface->pixels;

    int xc = x0;
    int yc = y * surface->pitch;
    while (xc++ < x1)
    {

        uint8_t bit7 = pattern & 0x80;

        /* set the green component */
        pixels[4 * xc + yc + 1] = bit7 ? 255 : 0;

        /* Rotate the mask. */
        pattern = (pattern << 1) | (bit7 ? 1 : 0);
    }
    SDL_UnlockSurface(surface);
}

void draw_vline(display_t *d, coord_t x, coord_t y0, coord_t y1, byte_t mode, byte_t pattern)
{

    SDL_Surface *surface = (SDL_Surface *)d->display_info;
    SDL_LockSurface(surface);

    uint8_t *pixels = (uint8_t *)surface->pixels;

    while (y0++ < y1)
    {

        uint8_t bit7 = pattern & 0x80;

        /* set the green component */
        pixels[surface->pitch * y0 + 4 * x + 1] = bit7 ? 255 : 0;

        /* Rotate the mask. */
        pattern = (pattern << 1) | (bit7 ? 1 : 0);
    }
    SDL_UnlockSurface(surface);
}

void draw_pixel(display_t *d, coord_t x0, coord_t y0, byte_t mode)
{
    SDL_Surface *surface = (SDL_Surface *)d->display_info;
    SDL_LockSurface(surface);
    uint8_t *pixels = (uint8_t *)surface->pixels;
    pixels[surface->pitch * y0 + 4 * x0 + 1] = 255;
    SDL_UnlockSurface(surface);
}

void draw_line(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;)
    { /* loop */
        draw_pixel(d, x0, y0, mode);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        } /* e_xy+e_x > 0 */
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        } /* e_xy+e_y < 0 */
    }
}

void draw_circle(display_t *d, coord_t x0, coord_t y0, coord_t radius, byte_t mode, byte_t pattern)
{
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    draw_pixel(d, x0, y0 + radius, mode);
    draw_pixel(d, x0, y0 - radius, mode);
    draw_pixel(d, x0 + radius, y0, mode);
    draw_pixel(d, x0 - radius, y0, mode);
    while (x < y)
    {
        /*  ddF_x == 2 * x + 1;
        ddF_y == -2 * y;
        f == x*x + y*y - radius*radius + 2*x - y + 1;
    */
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        draw_pixel(d, x0 + x, y0 + y, mode);
        draw_pixel(d, x0 - x, y0 + y, mode);
        draw_pixel(d, x0 + x, y0 - y, mode);
        draw_pixel(d, x0 - x, y0 - y, mode);
        draw_pixel(d, x0 + y, y0 + x, mode);
        draw_pixel(d, x0 - y, y0 + x, mode);
        draw_pixel(d, x0 + y, y0 - x, mode);
        draw_pixel(d, x0 - y, y0 - x, mode);
    }
}

void draw_ellipse(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern)
{
    int a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1;       /* values of diameter */
    long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
    long err = dx + dy + b1 * a * a, e2;                      /* error of 1.step */

    if (x0 > x1)
    {
        x0 = x1;
        x1 += a;
    } /* if called with swapped points */
    if (y0 > y1)
        y0 = y1; /* .. exchange them */
    y0 += (b + 1) / 2;
    y1 = y0 - b1; /* starting pixel */
    a *= 8 * a;
    b1 = 8 * b * b;
    do
    {
        draw_pixel(d, x1, y0, mode); /*   I. Quadrant */
        draw_pixel(d, x0, y0, mode); /*  II. Quadrant */
        draw_pixel(d, x0, y1, mode); /* III. Quadrant */
        draw_pixel(d, x1, y1, mode); /*  IV. Quadrant */
        e2 = 2 * err;
        if (e2 >= dx)
        {
            x0++;
            x1--;
            err += dx += b1;
        } /* x step */
        if (e2 <= dy)
        {
            y0++;
            y1--;
            err += dy += a;
        } /* y step */
    } while (x0 <= x1);
    while (y0 - y1 < b)
    {                                    /* too early stop of flat ellipses a=1 */
        draw_pixel(d, x0 - 1, y0, mode); /* -> finish tip of ellipse */
        draw_pixel(d, x1 + 1, y0++, mode);
        draw_pixel(d, x0 - 1, y1, mode);
        draw_pixel(d, x1 + 1, y1--, mode);
    }
}

void draw_rectangle(display_t *d, coord_t x0, coord_t y0, coord_t x1, coord_t y1, byte_t mode, byte_t pattern) {
    draw_hline(d,y0,x0,x1,mode,pattern);
    draw_hline(d,y1,x0,x1,mode,pattern);
    draw_vline(d,x0,y0,y1,mode,pattern);
    draw_vline(d,x1,y0,y1,mode,pattern);
}