#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int read_exact(const char *path, char *buf, int len) {
    int fd = open(path, O_RDONLY);
    int got;

    if (fd < 0) return -1;
    got = read(fd, buf, (size_t)len);
    close(fd);
    return got;
}

int main(void) {
    char buf[16];
    int fd;
    FILE *f;

    memset(buf, 0, sizeof(buf));
    if (read_exact("seed.txt", buf, 5) != 5) return 1;
    if (memcmp(buf, "alpha", 5) != 0) return 2;

    fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) return 3;
    if (write(fd, "xy", 2) != 2) return 4;
    if (close(fd) != 0) return 5;

    fd = open("out.txt", O_RDWR);
    if (fd < 0) return 6;
    if (lseek(fd, 1, SEEK_SET) != 1) return 7;
    if (write(fd, "Z", 1) != 1) return 8;
    if (lseek(fd, 0, SEEK_SET) != 0) return 9;
    memset(buf, 0, sizeof(buf));
    if (read(fd, buf, 2) != 2) return 10;
    if (memcmp(buf, "xZ", 2) != 0) return 11;
    if (close(fd) != 0) return 12;

    fd = open("out.txt", O_WRONLY | O_APPEND);
    if (fd < 0) return 13;
    if (lseek(fd, 0, SEEK_SET) != 0) return 14;
    if (write(fd, "!", 1) != 1) return 15;
    if (close(fd) != 0) return 16;

    memset(buf, 0, sizeof(buf));
    if (read_exact("out.txt", buf, 3) != 3) return 17;
    if (memcmp(buf, "xZ!", 3) != 0) return 18;

    if (rename("out.txt", "moved.txt") != 0) return 19;
    if (open("out.txt", O_RDONLY) >= 0) return 20;
    memset(buf, 0, sizeof(buf));
    if (read_exact("moved.txt", buf, 3) != 3) return 21;
    if (memcmp(buf, "xZ!", 3) != 0) return 22;
    if (unlink("moved.txt") != 0) return 23;
    if (open("moved.txt", O_RDONLY) >= 0) return 24;

    f = fopen("stdio.txt", "w+");
    if (!f) return 25;
    if (fwrite("abc", 1, 3, f) != 3) return 26;
    if (fseek(f, 0, SEEK_SET) != 0) return 27;
    memset(buf, 0, sizeof(buf));
    if (fread(buf, 1, 3, f) != 3) return 28;
    if (memcmp(buf, "abc", 3) != 0) return 29;
    if (fclose(f) != 0) return 30;
    if (remove("stdio.txt") != 0) return 31;

    return 0;
}
