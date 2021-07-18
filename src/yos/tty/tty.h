/*
 * tty.h
 *
 * tty functions
 * 
 * TODO:
 *  - scroll in all directions
 *  - special character codes handling i.e. \t \r
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

/* attributes for tty_attr */
#define AT_NONE         0x00
#define AT_UNDERLINE    0x01
#define AT_INVERSE      0x02

/* clear screen */
extern void tty_cls();

/* move cursor to x,y */
extern void tty_xy(uint8_t x, uint8_t y);

/* set attributes */
extern void tty_attr(uint8_t attr);

/* draw char */
extern void tty_outc(int c);

/* print char, respect \n */
extern void tty_putc(int c);

/* read keyboard (non-blocking!) */
extern int tty_getc();

/* scroll up 1 row */
extern void tty_scroll();

/* print string  */
extern void tty_puts(const char* s);

/* get string, max allowed chars are SSIZE_MAX */
extern void tty_gets(const char *s);

/* internal cursor function to xor cursor on screen */
extern void _tty_cur_tick();

/* enable/disable cursor */
extern void tty_cur_enable(bool enable);

#endif /* __TTY_H__ */