#include "xcc_exec_test.h"

unsigned char
same_six_bytes(const unsigned char *left, const unsigned char *right)
{
    unsigned int i;

    for (i = 0; i < 6; ++i) {
        if (left[i] != right[i])
            return 0;
    }
    return 1;
}

int
main(void)
{
    static const unsigned char first[6] = {3, 1, 4, 1, 5, 9};
    static const unsigned char equal[6] = {3, 1, 4, 1, 5, 9};
    static const unsigned char different[6] = {3, 1, 4, 2, 5, 9};

    XCC_CHECK_EQ_UINT_ID(1, same_six_bytes(first, equal), 1u);
    XCC_CHECK_EQ_UINT_ID(2, same_six_bytes(first, different), 0u);
    return 0;
}
