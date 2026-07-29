#include "xcc_exec_test.h"

extern unsigned int _div16(unsigned int a, unsigned int b);

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, _div16(60000u, 7u), 8571u);
    XCC_CHECK_EQ_UINT_ID(2, _div16(65535u, 255u), 257u);
    XCC_CHECK_EQ_UINT_ID(3, _div16(1024u, 32u), 32u);
    return 0;
}
