/*
 *	mouse.c
 *	mouse functions
 *
 *	13.04.2021  tstih
 */
#include "mouse.h"

/* static mouse info */
mouse_info_t mouse_info;

void mouse_init() {
	
    /* calibrate to center of screen (>>1 is div. by 2)*/
	mouse_calibrate(SCREEN_WIDTH>>1,SCREEN_HEIGHT>>1);

	/* scan for the first time */
	mouse_scan(&mouse_info);
	
	/* TODO: set current cursor */
}

#if __LINUX_SDL2__
void mouse_calibrate(word_t x, word_t y) {
    /* TODO: SDL_WarpMouse( x, y ); */ 
}

void mouse_scan(mouse_info_t *mouse_info) {
    /* TODO: SDL read mouse */
}
#endif /* __LINUX_SDL2__ */