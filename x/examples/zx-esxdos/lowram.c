/* Reclaim lower RAM only after BASIC has loaded and entered the program.
 * This program owns the machine until it halts; it cannot return to BASIC.
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile unsigned zx_disk_result;
volatile unsigned zx_disk_phase;

#define TRANSFER_BYTES 9216u
#define CHECK(n, condition) do { zx_disk_phase = (n); if (!(condition)) { \
    zx_disk_result = (n); for (;;) {} } } while (0)

static unsigned char pattern(unsigned position)
{
    return (unsigned char)((position * 37u) ^ (position >> 5) ^ 0xa5u);
}

int main(void)
{
    heap_t lower_heap;
    unsigned char *buffer;
    unsigned position;
    ssize_t count;
    int fd;

    puts("XCC: reclaiming 9 KiB below the loaded program");
    /* esxDOS 0.8.9 has already booted and BASIC has completed LOAD/USR.
     * The screen ends at 0x5b00; our code remains loaded at 0x8000.
     * Lower RAM is deliberately reclaimed here, never by the loader.
     */
    heap_init_arena(&lower_heap, (void *)0x5b00, (void *)0x8000);
    buffer = allocate(&lower_heap, TRANSFER_BYTES);
    CHECK(1, buffer != NULL);
    CHECK(2, (uintptr_t)buffer >= 0x5b00
          && (uintptr_t)buffer <= 0x8000 - TRANSFER_BYTES);
    for (position = 0; position < TRANSFER_BYTES; ++position)
        buffer[position] = pattern(position);

    fd = open("LOWRAM.DAT", O_WRONLY | O_CREAT | O_TRUNC);
    CHECK(3, fd >= 3);
    position = 0;
    while (position < TRANSFER_BYTES) {
        count = write(fd, buffer + position, TRANSFER_BYTES - position);
        CHECK(4, count > 0 && (unsigned)count <= TRANSFER_BYTES - position);
        position += (unsigned)count;
    }
    CHECK(5, fsync(fd) == 0);
    CHECK(6, close(fd) == 0);

    /* Reopen after flushing, and erase every byte before reading it back. */
    fd = open("LOWRAM.DAT", O_RDONLY);
    CHECK(7, fd >= 3);
    for (position = 0; position < TRANSFER_BYTES; ++position)
        buffer[position] = 0;
    position = 0;
    while (position < TRANSFER_BYTES) {
        count = read(fd, buffer + position, TRANSFER_BYTES - position);
        CHECK(8, count > 0 && (unsigned)count <= TRANSFER_BYTES - position);
        position += (unsigned)count;
    }
    for (position = 0; position < TRANSFER_BYTES; ++position)
        CHECK(9, buffer[position] == pattern(position));
    CHECK(10, read(fd, buffer, 1) == 0);
    CHECK(11, close(fd) == 0);
    free(buffer);

    /* free() discovers the owning arena; it also restores its capacity. */
    buffer = allocate(&lower_heap, TRANSFER_BYTES);
    CHECK(12, buffer != NULL);
    free(buffer);
    puts("Lower RAM disk round-trip: OK (9216 bytes)");
    zx_disk_result = 0xa55a;
    return 0;
}
