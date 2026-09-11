/* POSIX-style directory types exposed by the YOS service. */
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

DIR *opendir(const char *path);
struct dirent *readdir(DIR *directory);
void rewinddir(DIR *directory);
int closedir(DIR *directory);

#endif /* _YOS_DIRENT_H */
