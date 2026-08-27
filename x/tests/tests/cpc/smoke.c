#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef CPC_HAS_DISK
#define CPC_HAS_DISK 0
#endif

volatile uint8_t cpc_smoke_result;
volatile uint8_t cpc_smoke_phase = 0xee;
volatile off_t cpc_smoke_seek_result;
static unsigned initialized_value = 0x4937;
static unsigned zero_value;
static char initialized_text[] = "AMSTRAD";
static char *initialized_pointer = initialized_text;

static int compare_ints(const void *left, const void *right)
{
    int a = *(const int *)left;
    int b = *(const int *)right;
    return (a > b) - (a < b);
}

int fail(uint8_t code)
{
    cpc_smoke_result = code;
    puts("CPC LIBC FAIL");
    for (;;)
        cpc_smoke_result = cpc_smoke_result;
}

#if CPC_HAS_DISK
int disk_test(void)
{
    static const char text[] = "AMSDOS-C-LIBRARY";
    char copy[sizeof(text)];
    char tail[2];
    int fd = open("A:XTEST.DAT", O_WRONLY | O_CREAT | O_TRUNC);

    if (fd != 4 || write(fd, text, sizeof(text)) != sizeof(text)
        || close(fd) != 0)
        return 20;
    fd = open("A:XTEST.DAT", O_RDONLY);
    if (fd != 3)
        return 21;
    cpc_smoke_seek_result = lseek(fd, 2, SEEK_SET);
    if (cpc_smoke_seek_result != 2)
        return 25;
    if (lseek(fd, 1, SEEK_CUR) != 3
        || lseek(fd, -1, SEEK_CUR) != 2)
        return 29;
    if (read(fd, copy, sizeof(text) - 2) != sizeof(text) - 2)
        return 26;
    if (memcmp(copy, text + 2, sizeof(text) - 2) != 0)
        return 28;
    if (lseek(fd, -2, SEEK_END) != (off_t)(sizeof(text) - 2)
        || read(fd, tail, sizeof(tail)) != sizeof(tail)
        || memcmp(tail, text + sizeof(text) - 2, sizeof(tail)) != 0)
        return 30;
    if (close(fd) != 0)
        return 27;
    if (rename("A:XTEST.DAT", "A:XMOVED.DAT") != 0
        || open("A:XTEST.DAT", O_RDONLY) != -1)
        return 22;
    fd = open("A:XMOVED.DAT", O_RDONLY);
    if (fd != 3 || read(fd, copy, sizeof(text)) != sizeof(text)
        || close(fd) != 0 || memcmp(copy, text, sizeof(text)) != 0)
        return 23;
    if (unlink("A:XMOVED.DAT") != 0
        || open("A:XMOVED.DAT", O_RDONLY) != -1)
        return 24;
    if (unlink("A:XMOVED.DAT") != -1
        || rename("A:MISSING.DAT", "A:NEVER.DAT") != -1)
        return 35;

    FILE *stream = fopen("A:XSTDIO.DAT", "w");
    if (stream == NULL || fputs("ROM-STDIO", stream) == EOF
        || fclose(stream) != 0)
        return 31;
    stream = fopen("A:XSTDIO.DAT", "r");
    if (stream == NULL || fseek(stream, -5, SEEK_END) != 0
        || ftell(stream) != 4)
        return 32;
    if (fread(copy, 1, 5, stream) != 5
        || memcmp(copy, "STDIO", 5) != 0 || fclose(stream) != 0)
        return 33;
    if (remove("A:XSTDIO.DAT") != 0
        || fopen("A:XSTDIO.DAT", "r") != NULL)
        return 34;
    if (remove("A:XSTDIO.DAT") != -1)
        return 36;
    return 0;
}
#endif

int main(void)
{
    int values[] = {7, 1, 9, 3};
    const int key = 7;
    struct timespec ts = {12, 0};

    cpc_smoke_result = 1;
    cpc_smoke_phase = 0x10;
#if CPC_HAS_DISK
    int early_disk_result = disk_test();
    if (early_disk_result != 0)
        return fail((uint8_t)early_disk_result);
#endif
    if (initialized_value != 0x4937 || zero_value != 0
        || initialized_pointer != initialized_text
        || strcmp(initialized_text, "AMSTRAD") != 0)
        return fail(2);
    char *memory = calloc(16, 1);
    if (memory == NULL)
        return fail(3);
    strcpy(memory, "computer");
    memory = realloc(memory, 32);
    if (memory == NULL || strcmp(memory, "computer") != 0)
        return fail(4);
    memmove(memory + 1, memory, 8);
    if (memcmp(memory + 1, "computer", 8) != 0)
        return fail(5);
    free(memory);
    cpc_smoke_phase = 0x20;

    qsort(values, 4, sizeof(values[0]), compare_ints);
    if (*(int *)bsearch(&key, values, 4, sizeof(values[0]), compare_ints) != 7
        || strtol("-123", NULL, 10) != -123 || abs(-9) != 9)
        return fail(6);
    if (settimeofday(&ts) != 0 || timespec_get(&ts, TIME_UTC) != TIME_UTC
        || ts.tv_sec < 12 || ts.tv_sec > 13 || ts.tv_nsec != 0)
        return fail(7);
    cpc_smoke_phase = 0x30;

#if CPC_HAS_DISK
    cpc_smoke_phase = 0x35;
#else
    if (open("NOFILE", O_RDONLY) != -1
        || write(3, "x", 1) != -1 || close(1) != 0)
        return fail(8);
#endif

    while (trygetchar() != 0)
        ;
    cpc_smoke_result = 0x40;
    cpc_smoke_phase = 0x40;
    puts("PRESS Q");
    while (trygetchar() != 'q')
        ;
    puts("CPC STDLIB PASS");
    cpc_smoke_result = 0xa5;
    for (;;)
        cpc_smoke_result = cpc_smoke_result;
}
