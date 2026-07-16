#include "xcc_exec_test.h"

static unsigned int words[16];

int
main(void)
{
    unsigned int sum = 0;
    int index;

    for (index = 0; index < 16; ++index) {
        words[index] = (unsigned int)(index + 1);
        sum += words[index];
    }

    XCC_CHECK_EQ_UINT_ID(1, sum, 136u);
    XCC_CHECK_EQ_UINT_ID(2, words[0], 1u);
    XCC_CHECK_EQ_UINT_ID(3, words[15], 16u);
    return 0;
}
