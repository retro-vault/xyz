/*
 * Declares the tiny `printf`-style formatter layered on top of the tty
 * console output routines.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __TTY_PRINT_H__
#define __TTY_PRINT_H__

#include <stdarg.h>

#include <tty/tty.h>

/*
 * Scratch buffer length used by the formatter implementation.
 */
#define PRINT_BUF_LEN 128

/*
 * Internal formatting flags used by the tty printf implementation.
 */
enum flags {
	PAD_ZERO	= 1,
	PAD_RIGHT	= 2,
};

/*
 * Print formatted text to the tty console.
 */
extern void tty_printf(const char *format, ...);

#endif /* __TTY_PRINT_H__ */
