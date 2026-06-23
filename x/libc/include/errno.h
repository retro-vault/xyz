/*
 * errno.h
 *
 * Standard C error-reporting macros for the xcc Z80 target.
 *
 * The current libc is single-threaded, so errno is backed by one process-wide
 * integer object. Only the standard C error constants required by the core
 * language library are defined here.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _ERRNO_H
#define _ERRNO_H

/* Domain, range, and multibyte-conversion error codes. */
#define EDOM   33
#define ERANGE 34
#define EILSEQ 84

/* Backing storage for the errno macro. */
extern int __errno_value;

/* Modifiable lvalue designating the current error number. */
#define errno (__errno_value)

#endif /* _ERRNO_H */
