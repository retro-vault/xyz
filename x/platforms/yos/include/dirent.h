/* POSIX-style directory types exposed by the YOS service. */
#ifndef _YOS_DIRENT_H
#define _YOS_DIRENT_H

#include <yos.h>

#define DT_UNKNOWN YOS_DIRECTORY_TYPE_UNKNOWN
#define DT_DIR     YOS_DIRECTORY_TYPE_DIR
#define DT_REG     YOS_DIRECTORY_TYPE_REG

typedef yos_directory_t DIR;

struct dirent {
    ino_t d_ino;
    off_t d_size;
    unsigned char d_type;
    unsigned char d_attributes;
    char d_name[YOS_DIRECTORY_NAME_MAX + 1];
};

DIR *opendir(const char *path);
struct dirent *readdir(DIR *directory);
void rewinddir(DIR *directory);
int closedir(DIR *directory);

#endif /* _YOS_DIRENT_H */
