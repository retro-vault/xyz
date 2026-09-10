/*
 * sys/stat.h
 *
 * Minimal POSIX-style file metadata types and mode bits.
 *
 * The zx-esxdos backend supplies stat/fstat and mkdir. Other backends may
 * provide only the shared types and mode macros alongside open()/creat().
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

#define S_IFMT   0170000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_IFCHR  0020000

#define S_IRUSR  0400
#define S_IWUSR  0200
#define S_IXUSR  0100
#define S_IRGRP  0040
#define S_IWGRP  0020
#define S_IXGRP  0010
#define S_IROTH  0004
#define S_IWOTH  0002
#define S_IXOTH  0001

struct stat {
    dev_t   st_dev;
    ino_t   st_ino;
    mode_t  st_mode;
    nlink_t st_nlink;
    off_t   st_size;
};

#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)

/* Optional filesystem-backend operations (provided by zx-esxdos). */
[[sdcc::sdccall(1)]] int stat(const char *path, struct stat *buf);
[[sdcc::sdccall(1)]] int fstat(int fd, struct stat *buf);
[[sdcc::sdccall(1)]] int mkdir(const char *path, mode_t mode);

#endif /* _SYS_STAT_H */
