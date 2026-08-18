/*
 * sys.h — the libc platform contract.
 *
 * This is the complete set of hooks the target-independent C library needs from
 * a platform ("sys backend").  Port libc to a new machine by copying the `none`
 * backend (x/platforms/none/) and implementing each function below; see
 * x/platforms/none/README.md and x/docs/howtos/RETARGET-LIBC.md for the details and
 * reference (empty) implementations.
 *
 * All functions use XCC's [[sdcc::sdccall(1)]] convention (the first
 * word-sized arguments arrive in HL, DE, BC). A backend may be written in C
 * or assembly; the symbols are the same either way.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef XYZ_SYS_H
#define XYZ_SYS_H

#include <stddef.h>

struct timespec;   /* <time.h> */

/* ------------------------------------------------------------------------- *
 * Process
 * ------------------------------------------------------------------------- */

/* Terminate the program with `status`.  Does not return. */
[[sdcc::sdccall(1)]] void _exit(int status);

/* ------------------------------------------------------------------------- *
 * Heap
 *
 * Report the memory region the default libc heap should manage.  This is the
 * ONLY hook the allocator needs; malloc()/free() build a heap over it on first
 * use.  Special return convention (it is called from the allocator in asm):
 *
 *     returns  HL = base   (first usable byte)
 *              DE = limit  (one past the last usable byte)
 * ------------------------------------------------------------------------- */
[[sdcc::sdccall(1)]] void heap_region(void);

/* ------------------------------------------------------------------------- *
 * Wall clock  (see <time.h>)
 *
 * Fill *tv with the current time (seconds + nanoseconds since the Unix epoch)
 * and return 0, or return -1 if no clock is available.  set is optional.
 * ------------------------------------------------------------------------- */
[[sdcc::sdccall(1)]] int gettimeofday(struct timespec *tv);
[[sdcc::sdccall(1)]] int settimeofday(const struct timespec *tv); /* optional */

/* ------------------------------------------------------------------------- *
 * Byte I/O  (POSIX-style, on integer file descriptors)
 *
 * Descriptors 0/1/2 are the console (stdin/stdout/stderr); >= 3 are files.
 * Console output reaches the screen through write(1, ...).  A backend with no
 * filesystem implements only console write()/read() and returns errors for the
 * file operations.
 * ------------------------------------------------------------------------- */
[[sdcc::sdccall(1)]] int open(const char *path, int flags, int mode); /* fd >= 3, or -1 */
[[sdcc::sdccall(1)]] int close(int fd); /* 0, or -1 */
[[sdcc::sdccall(1)]] int read(int fd, void *buf, unsigned len); /* bytes, 0 = EOF, -1 */
[[sdcc::sdccall(1)]] int write(int fd, const void *buf, unsigned len); /* bytes, or -1 */
[[sdcc::sdccall(1)]] long lseek(int fd, long offset, int whence); /* new offset, or -1 */

/* Remove / rename a file.  Return 0 on success, -1 on failure. */
[[sdcc::sdccall(1)]] int unlink(const char *path);
[[sdcc::sdccall(1)]] int rename(const char *oldpath, const char *newpath);

#endif /* XYZ_SYS_H */
