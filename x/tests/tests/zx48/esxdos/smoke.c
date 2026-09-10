/* Exercise the public disk API, including its 32-bit stack arguments.
 * The same binary is run against real esxDOS and the hostile ABI model. */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/esxdos.h>
#include <unistd.h>

volatile unsigned zx_disk_result;
volatile unsigned zx_disk_phase;
volatile off_t zx_disk_position;

static int fail(unsigned phase)
{
    zx_disk_result = phase;
    for (;;) {}
}

#define CHECK(n, condition) do { zx_disk_phase = (n); if (!(condition)) return fail(n); } while (0)

int main(void)
{
    static const char text[] = "esxDOS/XCC: 0123456789";
    char copy[sizeof(text) + 8];
    int fd, second;
    struct stat status;
    FILE *stream;

    zx_disk_result = 0;
    CHECK(65, zx_esxdos_version() >= 0x0890);
    unlink("XDISK.TMP");
    unlink("XSTDIO.TMP");
    CHECK(1, open("XABSENT.TMP", O_RDONLY) == -1 && errno == ENOENT);
    fd = open("XDISK.TMP", O_RDWR | O_CREAT | O_EXCL);
    CHECK(2, fd >= 3);
    CHECK(3, write(fd, text, sizeof(text)) == sizeof(text));
    CHECK(4, fsync(fd) == 0);
    CHECK(5, lseek(fd, 0, SEEK_CUR) == sizeof(text));
    CHECK(6, lseek(fd, -10, SEEK_CUR) == sizeof(text) - 10);
    CHECK(7, read(fd, copy, 10) == 10 && memcmp(copy, text + sizeof(text) - 10, 10) == 0);
    CHECK(8, read(fd, copy, 1) == 0);
    CHECK(9, lseek(fd, -3, SEEK_END) == sizeof(text) - 3);
    CHECK(10, read(fd, copy, sizeof(copy)) == 3 && memcmp(copy, text + sizeof(text) - 3, 3) == 0);
    CHECK(11, lseek(fd, -1, SEEK_SET) == -1);
    CHECK(12, lseek(fd, 0, SEEK_SET) == 0);
    CHECK(13, lseek(fd, -1, SEEK_CUR) == -1);
    CHECK(14, lseek(fd, 0, 0x100) == -1);
    CHECK(15, read(fd, copy, 0) == 0 && write(fd, copy, 0) == 0);
    CHECK(16, close(fd) == 0);
    CHECK(17, close(fd) == -1 && errno == EBADF);
    CHECK(18, open("XDISK.TMP", O_RDWR | O_CREAT | O_EXCL) == -1 && errno == EEXIST);

    fd = open("XDISK.TMP", O_RDONLY);
    CHECK(19, fd >= 3);
    CHECK(20, write(fd, "!", 1) == -1);
    CHECK(21, close(fd) == 0);
    fd = open("XDISK.TMP", O_WRONLY | O_APPEND);
    CHECK(22, fd >= 3);
    CHECK(23, lseek(fd, 0, SEEK_SET) == 0);
    CHECK(24, write(fd, "A", 1) == 1);
    CHECK(25, lseek(fd, 0, SEEK_CUR) == sizeof(text) + 1);
    CHECK(26, close(fd) == 0);
    fd = open("XDISK.TMP", O_RDWR | O_TRUNC);
    CHECK(27, fd >= 3);
    CHECK(28, read(fd, copy, 1) == 0);

    /* More than 64 KiB proves that the wrapper does not discard offset's
     * upper word. Write consecutive 512-byte blocks so real FAT volumes
     * need no sparse-file behavior. */
    {
        static char block[512];
        unsigned i;
        memset(block, 0x5a, sizeof(block));
        for (i = 0; i < 129; ++i)
            CHECK(29, write(fd, block, sizeof(block)) == sizeof(block));
    }
    zx_disk_position = lseek(fd, 65537L, SEEK_SET);
    CHECK(30, zx_disk_position == 65537L);
    CHECK(31, write(fd, "Q", 1) == 1);
    CHECK(32, lseek(fd, -1L, SEEK_CUR) == 65537L);
    CHECK(33, read(fd, copy, 1) == 1 && copy[0] == 'Q');
    CHECK(34, lseek(fd, -511L, SEEK_END) == 65537L);
    CHECK(35, close(fd) == 0);
    CHECK(36, open("XABSENT.TMP", O_WRONLY | O_TRUNC) == -1);

    stream = fopen("XSTDIO.TMP", "w+");
    CHECK(37, stream != NULL);
    CHECK(38, fputs("stdio via esxDOS", stream) >= 0);
    CHECK(39, fflush(stream) == 0);
    CHECK(40, fseek(stream, -6L, SEEK_END) == 0 && ftell(stream) == 10L);
    CHECK(41, fread(copy, 1, 6, stream) == 6 && memcmp(copy, "esxDOS", 6) == 0);
    CHECK(42, fclose(stream) == 0);
    second = open("XSTDIO.TMP", O_RDONLY);
    CHECK(43, second >= 3);
    CHECK(44, close(second) == 0);
    CHECK(50, stat("XDISK.TMP", &status) == 0 && S_ISREG(status.st_mode)
          && status.st_size == 66048L);
    fd = open("XDISK.TMP", O_RDONLY);
    CHECK(51, fd >= 3 && fstat(fd, &status) == 0 && status.st_size == 66048L);
    CHECK(52, close(fd) == 0);
    CHECK(53, rename("XSTDIO.TMP", "XMOVED.TMP") == 0);
    CHECK(54, open("XSTDIO.TMP", O_RDONLY) == -1 && errno == ENOENT);
    CHECK(55, rename("XMOVED.TMP", "XSTDIO.TMP") == 0);
    CHECK(56, mkdir("XTESTDIR", 0777) == 0);
    CHECK(57, stat("XTESTDIR", &status) == 0 && S_ISDIR(status.st_mode) && status.st_size == 0);
    CHECK(58, chdir("XTESTDIR") == 0);
    {
        char directory[256];
        CHECK(59, getcwd(directory, sizeof(directory)) == directory);
        CHECK(60, strstr(directory, "XTESTDIR") != NULL);
        strcpy(copy, "guard");
        CHECK(61, getcwd(copy, 1) == NULL && errno == ERANGE && strcmp(copy, "guard") == 0);
    }
    CHECK(62, chdir("..") == 0);
    CHECK(63, rmdir("XTESTDIR") == 0);
    CHECK(64, stat("XTESTDIR", &status) == -1 && errno == ENOENT);
    CHECK(45, unlink("XDISK.TMP") == 0 && unlink("XSTDIO.TMP") == 0);
    CHECK(46, open("XDISK.TMP", O_RDONLY) == -1);
    CHECK(47, read(0x103, copy, 1) == -1 && errno == EBADF);
    CHECK(48, close(-1) == -1 && errno == EBADF);
    CHECK(49, close(0) == 0 && close(1) == 0 && close(2) == 0);
    zx_disk_result = 0xa55a;
    return 0;
}
