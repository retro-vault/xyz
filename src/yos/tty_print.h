/*
 * tty_print.h
 *
 * standard C file printf function
 * 
 * NOTES:
 *  Based on https://github.com/mpredfearn/simple-printf
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 02.05.2021   tstih
 *
 */
#ifndef __TTY_PRINT_H__
#define __TTY_PRINT_H__

#include <stdarg.h>
#include <tty.h>

#define PRINT_BUF_LEN 64

enum flags {
	PAD_ZERO	= 1,
	PAD_RIGHT	= 2,
};

/* THE printf */
extern void tty_printf(const char *format, ...);

#endif /* __TTY_PRINT_H__ */