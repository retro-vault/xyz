#include "xcc_exec_test.h"

int
main(void)
{
    unsigned int i;
    unsigned char hits = 0u;

    for (i = 0u; i < 17u; i = i + 1u)
        hits = (unsigned char)(hits + 1u);

    XCC_CHECK_EQ_UINT_ID(1, hits, 17u);
    return 0;
}
