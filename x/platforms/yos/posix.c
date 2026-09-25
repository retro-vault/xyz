#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <yos.h>

extern yos_t *yos_api_table;

_Static_assert(sizeof(yos_t) == 154, "YOS ABI 6 table size changed");
_Static_assert(sizeof(struct dirent) == sizeof(yos_directory_entry_t),
               "POSIX and YOS directory entries differ");

static int unavailable(void)
{
    errno = ENOSYS;
    return -1;
}

static void copy_yos_errno(void)
{
    if (yos_api_table && yos_api_table->error_number)
        errno = *yos_api_table->error_number;
}

int open(const char *path, int flags)
{
    int result;
    if (!yos_api_table || !yos_api_table->open)
        return unavailable();
    result = yos_api_table->open(path, flags);
    if (result < 0) copy_yos_errno();
    return result;
}

int close(int fd)
{
    int result;
    if (fd >= 0 && fd < 3)
        return 0;
    if (!yos_api_table || !yos_api_table->close)
        return unavailable();
    result = yos_api_table->close(fd);
    if (result < 0) copy_yos_errno();
    return result;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    ssize_t result;
    if (fd == STDIN_FILENO)
        return 0;
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        errno = EBADF;
        return -1;
    }
    if (!yos_api_table || !yos_api_table->read)
        return (ssize_t)unavailable();
    result = yos_api_table->read(fd, buffer, count);
    if (result < 0) copy_yos_errno();
    return result;
}

ssize_t write(int fd, const void *buffer, size_t count)
{
    const unsigned char *bytes = (const unsigned char *)buffer;
    size_t index;
    ssize_t result;

    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (!buffer && count) {
            errno = EFAULT;
            return -1;
        }
        for (index = 0; index < count; ++index)
            putchar(bytes[index]);
        return (ssize_t)count;
    }
    if (fd == STDIN_FILENO) {
        errno = EBADF;
        return -1;
    }
    if (!yos_api_table || !yos_api_table->write)
        return (ssize_t)unavailable();
    result = yos_api_table->write(fd, buffer, count);
    if (result < 0) copy_yos_errno();
    return result;
}

off_t lseek(int fd, off_t offset, int whence)
{
    off_t result;
    if (!yos_api_table || !yos_api_table->lseek) {
        unavailable();
        return (off_t)-1;
    }
    result = yos_api_table->lseek(fd, offset, whence);
    if (result < 0) copy_yos_errno();
    return result;
}

#define YOS_INT_CALL1(name) \
int name(const char *path) \
{ \
    int result; \
    if (!yos_api_table || !yos_api_table->name) return unavailable(); \
    result = yos_api_table->name(path); \
    if (result < 0) copy_yos_errno(); \
    return result; \
}

YOS_INT_CALL1(unlink)
YOS_INT_CALL1(chdir)
YOS_INT_CALL1(rmdir)

int rename(const char *old_path, const char *new_path)
{
    int result;
    if (!yos_api_table || !yos_api_table->rename) return unavailable();
    result = yos_api_table->rename(old_path, new_path);
    if (result < 0) copy_yos_errno();
    return result;
}

int fsync(int fd)
{
    int result;
    if (!yos_api_table || !yos_api_table->fsync) return unavailable();
    result = yos_api_table->fsync(fd);
    if (result < 0) copy_yos_errno();
    return result;
}

char *getcwd(char *buffer, size_t size)
{
    char *result;
    if (!yos_api_table || !yos_api_table->getcwd) {
        unavailable();
        return NULL;
    }
    result = yos_api_table->getcwd(buffer, size);
    if (!result) copy_yos_errno();
    return result;
}

int mkdir(const char *path, mode_t mode)
{
    int result;
    if (!yos_api_table || !yos_api_table->mkdir) return unavailable();
    result = yos_api_table->mkdir(path, mode);
    if (result < 0) copy_yos_errno();
    return result;
}

int stat(const char *path, struct stat *status)
{
    int result;
    if (!yos_api_table || !yos_api_table->stat) return unavailable();
    result = yos_api_table->stat(path, status);
    if (result < 0) copy_yos_errno();
    return result;
}

int fstat(int fd, struct stat *status)
{
    int result;
    if (!yos_api_table || !yos_api_table->fstat) return unavailable();
    result = yos_api_table->fstat(fd, status);
    if (result < 0) copy_yos_errno();
    return result;
}

DIR *opendir(const char *path)
{
    DIR *result;
    if (!yos_api_table || !yos_api_table->opendir) {
        unavailable();
        return NULL;
    }
    result = yos_api_table->opendir(path);
    if (!result) copy_yos_errno();
    return result;
}

struct dirent *readdir(DIR *directory)
{
    struct dirent *result;
    if (!yos_api_table || !yos_api_table->readdir) {
        unavailable();
        return NULL;
    }
    result = (struct dirent *)yos_api_table->readdir(directory);
    if (!result) copy_yos_errno();
    return result;
}

void rewinddir(DIR *directory)
{
    if (yos_api_table && yos_api_table->rewinddir)
        yos_api_table->rewinddir(directory);
    else
        unavailable();
}

int closedir(DIR *directory)
{
    int result;
    if (!yos_api_table || !yos_api_table->closedir) return unavailable();
    result = yos_api_table->closedir(directory);
    if (result < 0) copy_yos_errno();
    return result;
}
