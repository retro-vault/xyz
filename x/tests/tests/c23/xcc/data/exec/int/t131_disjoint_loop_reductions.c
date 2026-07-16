#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned int
two_reductions(const unsigned char *cursor)
{
    unsigned int first = 0;
    unsigned int second = 0;
    int index = 0;

    while (index < 16) {
        first += *cursor++;
        ++index;
    }

    index = 0;
    while (index < 16) {
        second += *cursor++;
        ++index;
    }
    return first + second;
}

int
main(void)
{
    static const unsigned char values[32] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32
    };

    XCC_CHECK_EQ_UINT_ID(1, two_reductions(values), 528u);
    return 0;
}
