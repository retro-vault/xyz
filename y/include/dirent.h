/*
 * POSIX-style directory types exposed by the YOS service.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _YOS_DIRENT_H
#define _YOS_DIRENT_H

#include <sys/types.h>

#define DT_UNKNOWN 0
#define DT_DIR     4
#define DT_REG     8

#define YOS_NAME_MAX 12

typedef struct yos_directory DIR;

struct dirent {
    ino_t d_ino;
    off_t d_size;
    unsigned char d_type;
    unsigned char d_attributes;
    char d_name[YOS_NAME_MAX + 1];
};

#endif /* _YOS_DIRENT_H */
