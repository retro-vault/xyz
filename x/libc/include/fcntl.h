/*
 * fcntl.h
 *
 * Minimal POSIX-style file status flags for the xcc Z80 libc.
 *
 * The CP/M 3 backend currently honours the access, create, truncate, and
 * append bits below. Permission bits are accepted for source compatibility
 * but are ignored by the backend.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/types.h>

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_ACCMODE 0x0003

#define O_CREAT  0x0100
#define O_TRUNC  0x0200
#define O_APPEND 0x0400

#define O_BINARY 0x0000
#define O_TEXT   0x0000

/* No file modes on the supported platforms: open takes exactly two
 * arguments so it can use the register calling convention that the
 * assembly backends and stdio expect. */
int open(const char *path, int oflag);

#define creat(path, mode) open((path), O_WRONLY | O_CREAT | O_TRUNC)

#endif /* _FCNTL_H */
