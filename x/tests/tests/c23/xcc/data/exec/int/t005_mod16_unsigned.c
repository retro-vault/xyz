#include "xcc_exec_test.h"

extern unsigned int _mod16(unsigned int a, unsigned int b);

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, _mod16(60000u, 7u), 3u);
    XCC_CHECK_EQ_UINT_ID(2, _mod16(65535u, 255u), 0u);
    XCC_CHECK_EQ_UINT_ID(3, _mod16(1025u, 32u), 1u);
    return 0;
}
