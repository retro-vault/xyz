/*
 * stdarg.h
 *
 * Standard C23 variadic argument support for the xcc Z80 target.
 *
 * xcc places arguments on the stack in declaration order as seen from the
 * callee's frame, so a variadic list can be represented as a byte pointer to
 * the next unread argument slot.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDARG_H
#define _STDARG_H

#define __STDC_VERSION_STDARG_H__ 202311L

typedef char *va_list;

/* Initialize a variadic cursor just past the last named argument. */
#define va_start(ap, last) \
    ((ap) = (va_list)((char *)(&(last)) + sizeof(last)))

/* Read the current argument as type T and advance the cursor. */
#define va_arg(ap, T) \
    (*(T *)((ap) += sizeof(T), (ap) - sizeof(T)))

/* No target-specific cleanup is required for this ABI. */
#define va_end(ap) ((void)0)

/* Copy one variadic cursor to another. */
#define va_copy(dst, src) ((dst) = (src))

#endif /* _STDARG_H */
