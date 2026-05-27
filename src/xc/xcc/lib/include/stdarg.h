/*
 * stdarg.h — variadic argument macros for xcc (Z80 target).
 *
 * All parameters are pushed right-to-left onto the stack (xcc ABI),
 * so inside a function the named parameters live at positive IX offsets
 * (IX+4, IX+6, ...) and variadic arguments continue immediately after
 * the last named parameter.
 *
 * va_list is a char* pointing to the next argument slot.  va_start
 * sets it to the address just past the last named parameter.
 * va_arg reads the current slot and advances the pointer.
 *
 * Usage:
 *   #include <stdarg.h>
 *   void log(const char *fmt, ...) {
 *       va_list ap;
 *       va_start(ap, fmt);
 *       int x = va_arg(ap, int);
 *       va_end(ap);
 *   }
 *
 * Preprocess with: xcc -I/usr/local/lib/xcc/include ...
 * or              cpp -I/usr/local/lib/xcc/include ... | xcc -
 */

#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

/* Set ap to point past the last named parameter. */
#define va_start(ap, last) \
    ((ap) = (va_list)((char *)(&(last)) + sizeof(last)))

/* Fetch the next argument of type T and advance ap. */
#define va_arg(ap, T) \
    (*(T *)((ap) += sizeof(T), (ap) - sizeof(T)))

/* No-op on Z80 (no hardware state to restore). */
#define va_end(ap) ((void)0)

/* Copy a va_list. */
#define va_copy(dst, src) ((dst) = (src))

#endif /* _STDARG_H */
