#include "xcc_exec_test.h"

static const unsigned char samples[7] = {1, 4, 9, 16, 25, 36, 49};

unsigned int
fold_bytes(const unsigned char *cursor)
{
    unsigned int accumulator = 257;
    unsigned int i;

    for (i = 0; i < 7; ++i)
        accumulator = (unsigned int)((accumulator << 3) + accumulator +
                                     cursor[i]);
    return accumulator;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, fold_bytes(samples), 18597u);
    return 0;
}
