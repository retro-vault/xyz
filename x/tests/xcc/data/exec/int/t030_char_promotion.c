#include "xcc_exec_test.h"

extern unsigned int __mul16(unsigned int a, unsigned int b);
extern unsigned int __div16(unsigned int a, unsigned int b);
extern unsigned int __mod16(unsigned int a, unsigned int b);

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, __div16(207u, 3u), 69u);
    XCC_CHECK_EQ_UINT_ID(2, __mul16(7u, 9u), 63u);
    XCC_CHECK_EQ_UINT_ID(3, __div16(250u, 10u), 25u);
    XCC_CHECK_EQ_UINT_ID(4, __mod16(250u, 7u), 5u);
    return 0;
}
