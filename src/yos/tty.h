/*
 * tty.h
 *
 * tty functions
 * 
 * TODO:
 *  - scroll in all directions
 *  - cursor handling
 *  - special character codes handling i.e. \n \r
 *  - speccy keybord E mode (caps+sym)
 *  - speccy keyboard auto repeat
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2016-08-26   tstih
 *
 */
#ifndef __TTY_H__
#define __TTY_H__

#include <stdint.h>
#include <stdbool.h>

extern void * tty_font;
extern uint8_t * tty_x;
extern uint8_t * tty_y;

/* clear screen */
extern void tty_cls();

/* move cursor to x,y */
extern void tty_xy(uint8_t x, uint8_t y);

/* print char */
extern void tty_putc(int c);

/* read keyboard (blocking!) */
extern int tty_getc();

/* scroll up 1 row */
extern void tty_scroll();

/* print string  */
extern void tty_puts(const char* s);

#endif /* __TTY_H__ */
