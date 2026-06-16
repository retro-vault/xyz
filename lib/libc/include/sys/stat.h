/*
 * sys/stat.h
 *
 * Minimal POSIX-style file metadata types and mode bits.
 *
 * The current CP/M 3 backend does not yet expose full stat/fstat entry
 * points, but these definitions let portable code use the standard types
 * and creation-mode macros alongside open()/creat().
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

#endif /* _SYS_STAT_H */
