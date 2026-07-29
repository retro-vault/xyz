#include "xcc_exec_test.h"

static unsigned char steps[16] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

__attribute__((noinline)) unsigned
walk_squares(void)
{
    unsigned index = 2;
    unsigned square = 4;

    while (square < 100) {
        square += index + index + steps[index];
        ++index;
    }
    return index;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, walk_squares(), 10);
    return 0;
}
