#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int compare(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}

int main(void)
{
    static const char message[] = "CPC 6128 full libc and AMSDOS\n";
    int values[] = {8, 1, 6, 3};
    char copy[sizeof(message)];
    int fd;

    qsort(values, 4, sizeof(values[0]), compare);
    if (values[0] != 1 || values[3] != 8)
        return 1;
    fd = open("A:result.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0 || write(fd, message, sizeof(message)) != sizeof(message)
        || close(fd) != 0)
        return 2;
    fd = open("A:result.txt", O_RDONLY);
    if (fd < 0 || read(fd, copy, sizeof(copy)) != sizeof(copy)
        || close(fd) != 0 || strcmp(copy, message) != 0)
        return 3;
    puts("CPC 6128 library test passed");
    return 0;
}
