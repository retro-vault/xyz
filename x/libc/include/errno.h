/*
 * errno.h
 *
 * Standard C error-reporting macros for the xcc Z80 target.
 *
 * The current libc is single-threaded, so errno is backed by one process-wide
 * integer object. Standard C and POSIX-style backend errors are defined here.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _ERRNO_H
#define _ERRNO_H

/* POSIX-style backend errors, plus the standard C error codes. */
#define EPERM     1
#define ENOENT    2
#define EIO       5
#define ENXIO     6
#define EBADF     9
#define ENOMEM   12
#define EACCES   13
#define EFAULT   14
#define EBUSY    16
#define EEXIST   17
#define ENODEV   19
#define ENOTDIR  20
#define EISDIR   21
#define EINVAL   22
#define EMFILE   24
#define EFBIG    27
#define ENOSPC   28
#define ESPIPE   29
#define EROFS    30
#define EDOM   33
#define ERANGE 34
#define ENAMETOOLONG 36
#define ENOSYS  38
#define ENOTEMPTY 39
#define EOVERFLOW 75
#define EILSEQ 84

/* Backing storage for the errno macro. */
extern int _errno_value;

/* Modifiable lvalue designating the current error number. */
#define errno (_errno_value)

#endif /* _ERRNO_H */
