/*
 * unistd.h
 *
 * Small POSIX-style low-level I/O declarations for the xcc Z80 libc.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/types.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

[[sdcc::sdccall(1)]] int close(int fd);
[[sdcc::sdccall(1)]] off_t lseek(int fd, off_t offset, int whence);
[[sdcc::sdccall(1)]] ssize_t read(int fd, void *buf, size_t count);
[[sdcc::sdccall(1)]] ssize_t write(int fd, const void *buf, size_t count);
[[sdcc::sdccall(1)]] int unlink(const char *path);
void *sbrk(ptrdiff_t increment);

#endif /* _UNISTD_H */
