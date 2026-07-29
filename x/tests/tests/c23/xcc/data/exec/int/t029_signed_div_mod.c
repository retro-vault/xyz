#include "xcc_exec_test.h"

extern unsigned int _div16(unsigned int a, unsigned int b);
extern unsigned int _mod16(unsigned int a, unsigned int b);

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, _div16(1234u, 17u), 72u);
    XCC_CHECK_EQ_UINT_ID(2, _mod16(1234u, 17u), 10u);
    XCC_CHECK_EQ_UINT_ID(3, _div16(65530u, 10u), 6553u);
    XCC_CHECK_EQ_UINT_ID(4, _mod16(65530u, 10u), 0u);
    return 0;
}
