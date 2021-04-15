/*
 * mouse.c
 *
 * basic mouse functions (just control, no drawing!)
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
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
#if __LINUX_SDL2__
    /* no need for SDL_WarpMouse, SDL handles mouse position automatically */
#endif
    /* TODO: */ 
}

void mouse_scan(mouse_info_t *mouse_info) {
#if __LINUX_SDL2__
    int x,y,sts;
    sts=SDL_GetMouseState(&x, &y);
    mouse_info->rect.x0=x;
    mouse_info->rect.y0=y;
    mouse_info->status=0;
    if (sts & SDL_BUTTON(SDL_BUTTON_LEFT))
        mouse_info->status|=MOUSE_LBUTTON;
    if (sts & SDL_BUTTON(SDL_BUTTON_RIGHT))
        mouse_info->status|=MOUSE_RBUTTON;
#endif
    /* TODO: mouse_info rect, zx spectrum impl. */
}
#endif /* __LINUX_SDL2__ */