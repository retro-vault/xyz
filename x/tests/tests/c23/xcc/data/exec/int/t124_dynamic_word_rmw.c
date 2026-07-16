#include "xcc_exec_test.h"

static unsigned int bins[32];

static unsigned int
dynamic_histogram(void)
{
    unsigned int seed = 0x5a17u;
    unsigned int checksum = 0;

    for (unsigned int i = 0; i < 513u; ++i) {
        seed = (unsigned int)(seed * 109u + 89u);
        bins[(seed >> 4) & 31u]++;
    }
    for (unsigned int i = 0; i < 32u; ++i)
        checksum = (unsigned int)(checksum + bins[i] * (i + 1u));
    return checksum;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, dynamic_histogram(), 8451u);
    return 0;
}
