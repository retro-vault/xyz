/* Keep reclaimed Spectrum RAM live across real esxDOS file operations.
 * The 32 KiB allocation cannot fit in the old 0x8000..0xEFFF heap.
 * MIT License (see: LICENSE). Copyright (C) 2026 tomaz stih.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile unsigned zx_disk_result;
volatile unsigned zx_disk_phase;

#define CHECK(n, condition) do { zx_disk_phase = (n); if (!(condition)) { \
    zx_disk_result = (n); for (;;) {} } } while (0)

static unsigned char pattern(unsigned offset)
{
    return (unsigned char)(offset ^ (offset >> 8) ^ 0xa7u);
}

int main(void)
{
    unsigned char *data;
    unsigned char *guard;
    struct stat status;
    unsigned i;
    int fd;

    data = malloc(32768u);
    CHECK(1, data != NULL);
    CHECK(2, (uintptr_t)data >= 0x5b00u && (uintptr_t)data < 0x5c00u);
    guard = malloc(4096u);
    CHECK(3, guard != NULL);
    memset(guard, 0x6d, 4096u);
    for (i = 0; i < 32768u; ++i)
        data[i] = pattern(i);

    unlink("XHEAP.TMP");
    unlink("XHEAP2.TMP");
    fd = open("XHEAP.TMP", O_CREAT | O_EXCL | O_RDWR);
    CHECK(4, fd >= 3);
    CHECK(5, write(fd, data, 32767u) == 32767);
    CHECK(6, write(fd, data + 32767u, 1) == 1);
    CHECK(7, fsync(fd) == 0);
    CHECK(8, fstat(fd, &status) == 0 && status.st_size == 32768L);
    CHECK(9, lseek(fd, 0, SEEK_SET) == 0);
    memset(data, 0, 32768u);
    CHECK(10, read(fd, data, 32767u) == 32767);
    CHECK(11, read(fd, data + 32767u, 1) == 1);
    CHECK(12, close(fd) == 0);
    CHECK(13, rename("XHEAP.TMP", "XHEAP2.TMP") == 0);
    CHECK(14, stat("XHEAP2.TMP", &status) == 0
              && status.st_size == 32768L);
    CHECK(15, unlink("XHEAP2.TMP") == 0);
    for (i = 0; i < 32768u; ++i)
        CHECK(16, data[i] == pattern(i));
    for (i = 0; i < 4096u; ++i)
        CHECK(17, guard[i] == 0x6d);
    free(guard);
    free(data);

    /* Also verify coalescing leaves the reclaimed arena reusable. */
    data = malloc(36864u);
    CHECK(18, data != NULL && (uintptr_t)data < 0x5c00u);
    data[0] = 0x53;
    data[36863u] = 0xca;
    CHECK(19, data[0] == 0x53 && data[36863u] == 0xca);
    free(data);
    zx_disk_result = 0xa55a;
    return 0;
}
