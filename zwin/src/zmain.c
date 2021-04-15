/*
 * zmain.c
 *
 * entry, exit and loop functions z windows 
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#include "zmain.h"

boolean_t zmain_start() {

#if __LINUX_SDL2__
    /* initialize sdl */
    if (!zmain_sdl_start()) return FALSE;
#endif

    /* initialize screen graphics
     */
    graphics_t *g=graphics_init();

    /* initialize messaging system
     * this will create the message queue.
     */
    message_init();

    /* initialize windowing system
     * this will initialize graphics and create
     * desktop window
     */
    window_init();

    /* initialize mouse 
     * this will calibrate the mouse to the center 
     * of the screen and execute first mouse hw scan.
    */
    mouse_init();

    /* TODO: create desktop window, it will repaint itself */
    int y;
    for (y=0;y<SCREEN_HEIGHT;y++)
        draw_hline(g, y, y%2, SCREEN_WIDTH-1-y%2,DWM_SET,0xaa);

    /* we won! */
    return TRUE;
}

void zmain_end() {

#if __LINUX_SDL2__
    /* finalize sdl */
    zmain_sdl_end();
#endif

}

void zmain_loop() {

    while(TRUE) {

#if __LINUX_SDL2__
        /* main sdl loop */
        if (!zmain_sdl_loop()) return;
#endif

    }

}

/* harvest and dispatch events
 * tis is zwin's main loop. it harvests events and
 * dispatches them to correct windows' message queues.
 */
int main(int argc, char *args[]) {
    if (zmain_start()) {
        zmain_loop();
        zmain_end();
    }
    return 0;
}