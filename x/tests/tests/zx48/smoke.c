#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

volatile uint8_t zx_smoke_result;
volatile char zx_smoke_input[4];
static unsigned initialized_value = 0x4937;
static unsigned zero_value;
static char initialized_text[] = "TAMSYN";
static char *initialized_pointer = initialized_text;

static int compare_ints(const void *lhs, const void *rhs)
{
    const int a = *(const int *)lhs;
    const int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static int fail(uint8_t code)
{
    zx_smoke_result = code;
    puts("ZX48 FAIL");
    return code;
}

int main(void)
{
    int values[] = {7, 1, 9, 3};
    int polled_key;

    zx_smoke_result = 1;
    if (initialized_value != 0x4937 || zero_value != 0
        || initialized_pointer != initialized_text
        || strcmp(initialized_text, "TAMSYN") != 0)
        return fail(2);

    char *memory = calloc(16, 1);
    if (memory == NULL)
        return fail(3);
    strcpy(memory, "spectrum");
    memory = realloc(memory, 32);
    if (memory == NULL || strcmp(memory, "spectrum") != 0)
        return fail(4);
    memmove(memory + 1, memory, 8);
    if (memcmp(memory + 1, "spectrum", 8) != 0)
        return fail(5);
    free(memory);

    qsort(values, 4, sizeof(values[0]), compare_ints);
    const int key = 7;
    if (*(int *)bsearch(&key, values, 4, sizeof(values[0]), compare_ints) != 7)
        return fail(6);
    if (strtol("-123", NULL, 10) != -123 || abs(-9) != 9)
        return fail(7);

    struct timespec ts;
    if (time(NULL) != (time_t)-1 || clock() != (clock_t)-1
        || timespec_get(&ts, TIME_UTC) != 0)
        return fail(8);
    if (open("none", O_RDONLY) != -1)
        return fail(9);
    if (write(3, "x", 1) != -1)
        return fail(10);
    if (close(1) != 0)
        return fail(11);
    if (trygetchar() != 0)
        return fail(12);

    for (unsigned line = 0; line != 18; ++line)
        puts("TAMSYN CONSOLE");
    puts("POLL q");
    do {
        polled_key = trygetchar();
    } while (polled_key == 0);
    if (polled_key != 'q')
        return fail(13);
    while (trygetchar() != 0)
        ;
    puts("TYPE aA! ENTER");
    if (read(0, (void *)zx_smoke_input, sizeof(zx_smoke_input))
        != sizeof(zx_smoke_input))
        return fail(14);
    for (unsigned index = 0; index != sizeof(zx_smoke_input); ++index) {
        if (zx_smoke_input[index] != "aA!\r"[index])
            return fail((uint8_t)(15 + index));
    }

    puts("ZX48 STDLIB PASS");
    zx_smoke_result = 0xa5;
    return 0;
}
