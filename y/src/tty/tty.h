/*
 * Declares the text console and keyboard-facing tty helpers exposed by
 * the YOS terminal subsystem.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __TTY_H__
#define __TTY_H__

#include <stdint.h>
#include <stdbool.h>

/*
 * Default text attribute with no extra styling.
 */
#define AT_NONE         0x00
/*
 * Attribute bit that enables underline rendering.
 */
#define AT_UNDERLINE    0x01
/*
 * Attribute bit that enables inverse video rendering.
 */
#define AT_INVERSE      0x02

/*
 * Clear the text screen.
 */
extern void tty_cls(void);

/*
 * Move the text cursor to the supplied column and row.
 */
extern void tty_xy(uint8_t x, uint8_t y);

/*
 * Set the active text attribute bits.
 */
extern void tty_attr(uint8_t attr);

/*
 * Draw one character without applying tty newline behavior.
 */
extern void tty_outc(int c);

/*
 * Print one character and honor tty control handling such as newline.
 */
extern void tty_putc(int c);

/*
 * Read one queued keyboard character without blocking.
 */
extern int tty_getc(void);

/*
 * Scroll the screen up by one row.
 */
extern void tty_scroll(void);

/*
 * Print a zero-terminated string.
 */
extern void tty_puts(const char* s);

/*
 * Read one line of input into the supplied buffer.
 */
extern void tty_gets(const char *s);

/*
 * Timer-driven internal cursor blink hook.
 */
extern void _tty_cur_tick(void);
#define __tty_cur_tick _tty_cur_tick

/*
 * Enable or disable the visible text cursor.
 */
extern void tty_cur_enable(bool enable);

#endif /* __TTY_H__ */
