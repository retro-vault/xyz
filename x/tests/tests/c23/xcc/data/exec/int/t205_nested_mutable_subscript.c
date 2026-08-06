#include "xcc_exec_test.h"

static const unsigned char samples[13] = {
    9, 2, 8, 1, 7, 3, 6, 4, 5, 0, 10, 2, 11
};
static const unsigned char limits[13] = {
    0, 4, 0, 5, 2, 8, 1, 5, 3, 9, 4, 7, 6
};
static const unsigned char expected[13] = {
    0, 0, 0, 2, 4, 5, 4, 6, 6, 9, 10, 10, 12
};
static volatile unsigned char observed[13];

static __attribute__((noinline)) void
scan_prefixes(void)
{
    unsigned char outer;

    for (outer = 1; outer < 13; ++outer) {
        unsigned char inner = outer;
        unsigned char limit = limits[outer];

        while (inner > 0 && samples[inner - 1] > limit)
            --inner;
        observed[outer] = inner;
    }
}

int
main(void)
{
    unsigned char i;

    scan_prefixes();
    for (i = 1; i < 13; ++i) {
        if (observed[i] != expected[i])
            return i;
    }
    return 0;
}
