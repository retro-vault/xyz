/*
 * tty_print.c
 *
 * standard C file printf function
 * 
 * NOTES:
 *  Inspiration https://github.com/mpredfearn/simple-printf
 *  Removed support for %h
 *  Removed support for long and long long
 *  Removed char count
 *  
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 02.05.2021   tstih
 *
 */
#include <tty/tty_print.h>

/* prints the string of width with flags */
static void _prints(const char *string, unsigned char width, unsigned char flags)
{
	char padchar = ' ';

	if (width > 0) {
		unsigned char len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (flags & PAD_ZERO)
			padchar = '0';
	}
	if (!(flags & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			tty_putc(padchar);
		}
	}
	for ( ; *string ; ++string) {
		tty_putc(*string);
	}
	for ( ; width > 0; --width) {
		tty_putc(padchar);
	}
}

/* print the integer */
static void _printi(
    int i,
    unsigned char base,
    unsigned char sign,
    unsigned char width,
    unsigned char flags,
    unsigned char letbase)
{
	char print_buf[PRINT_BUF_LEN];
	char *s;
	unsigned char t, neg = 0;
	unsigned int u = i;
	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		_prints(print_buf, width, flags);
        return;
	}
	if (sign && base == 10 && i < 0) {
		neg = 1;
		u = -i;
	}
	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';
	while (u) {
		t = u % base;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= base;
	}
	if (neg) {
		if( width && (flags & PAD_ZERO) ) {
			tty_putc ('-');
			--width;
		}
		else {
			*--s = '-';
		}
	}
	_prints(s, width, flags);
}

void tty_printf(const char *format, ...)
{
    /* handle variable args */
    va_list ap;
    va_start(ap, format);

	unsigned char width, flags;
	char scr[2];
    unsigned int value;
    char *str;
    unsigned char base, sign, letbase;
    char ch;

    /* for each char in format */
	for (; *format != 0; ++format) {
        /* if it is formatting */
		if (*format == '%') {
			++format;                   /* peek at next char after % */
			width = flags = 0;
			if (*format == '\0')        /* if end of string it's a mistake: ignore */
				break;
			if (*format == '%')         /* if %% then it's escape code */
				goto esc;
			if (*format == '-') {       /* if - then pad right and get next format specifier */
				++format;
				flags = PAD_RIGHT;
			}
			while (*format == '0') {    /* if 0 then pad zero and get next format specifier */
				++format;
				flags |= PAD_ZERO;
			}
			if (*format == '*') {
				width = va_arg(ap, int);
				format++;
			} else {
				for ( ; *format >= '0' && *format <= '9'; ++format) {
					width *= 10;
					width += *format - '0';
				}
			}
            /* "main" format specifier */
			switch (*format) {
				case('d'):              /* decimal! */
					value = (unsigned int)va_arg(ap, int);
                    base = 10;
                    sign = 1;
                    letbase = 'a';
                    goto print_number;

				case('u'):              /* unsigned */
					value = va_arg(ap, unsigned int);
                    base = 10;
                    sign = 0;
                    letbase = 'a';
                    goto print_number;

				case('x'):              /* hex */
					value = va_arg(ap, unsigned int);
                    base = 16;
                    sign = 0;
                    letbase = 'a';
                    goto print_number;

				case('X'):              /* hex, capital */
					value = va_arg(ap, unsigned int);
                    base = 16;
                    sign = 0;
                    letbase = 'A';
print_number:
					_printi((int)value, base, sign, width, flags, letbase);
					break;

				case('c'):              /* char */
					ch = (char)va_arg(ap, int);
					scr[0] = ch;
					scr[1] = '\0';
					_prints(scr, width, flags);
					break;

				case('s'):              /* string */
					str = va_arg(ap, char *);
                    #pragma disable_warning 196
					_prints(str ? str : "(null)", width, flags);
					break;

				default:
					break;
			}
		} else { /* char is not formatting, just display it */
esc:
			tty_putc(*format);
		}
	}
}
