#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    static const char text[] = "Hello from XCC and esxDOS!\n";
    char buffer[sizeof text];
    unsigned done;
    ssize_t count;
    int fd;

    puts("XCC esxDOS disk example");
    fd = open("XCCDISK.TXT", O_RDWR | O_CREAT | O_TRUNC);
    if (fd < 0) {
        perror("open XCCDISK.TXT");
        return 1;
    }

    done = 0;
    while (done < sizeof text - 1) {
        count = write(fd, text + done, sizeof text - 1 - done);
        if (count <= 0) {
            puts("File write did not complete");
            close(fd);
            return 1;
        }
        done += (unsigned)count;
    }
    if (fsync(fd) < 0 || lseek(fd, 0L, SEEK_SET) < 0) {
        perror("flush or seek");
        close(fd);
        return 1;
    }

    done = 0;
    while (done < sizeof text - 1) {
        count = read(fd, buffer + done, sizeof text - 1 - done);
        if (count <= 0) {
            puts("File read did not complete");
            close(fd);
            return 1;
        }
        done += (unsigned)count;
    }
    buffer[done] = '\0';
    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    if (memcmp(buffer, text, sizeof text) != 0) {
        puts("File contents did not match");
        return 1;
    }

    puts(buffer);
    puts("Disk round-trip: OK");
    puts("XCCDISK.TXT remains on the current disk.");
    return 0;
}
