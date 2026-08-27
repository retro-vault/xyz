#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    static const char message[] = "CPC 664 AMSDOS file I/O\n";
    char copy[sizeof(message)];
    int fd = open("A:result.txt", O_WRONLY | O_CREAT | O_TRUNC);

    if (fd < 0 || write(fd, message, sizeof(message)) != sizeof(message)
        || close(fd) != 0) {
        puts("write failed");
        return 1;
    }
    fd = open("A:result.txt", O_RDONLY);
    if (fd < 0 || read(fd, copy, sizeof(copy)) != sizeof(copy)
        || close(fd) != 0 || memcmp(copy, message, sizeof(message)) != 0) {
        puts("read failed");
        return 2;
    }
    puts("AMSDOS file round trip passed");
    return 0;
}
