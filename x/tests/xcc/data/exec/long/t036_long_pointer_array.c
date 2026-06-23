#include "xcc_exec_test.h"

extern unsigned long __div32(unsigned long a, unsigned long b);
extern unsigned long __mod32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_ULONG_ID(1, __div32(987654321ul, 9ul), 109739369ul);
    XCC_CHECK_EQ_ULONG_ID(2, __mod32(987654321ul, 9ul), 0ul);
    XCC_CHECK_EQ_ULONG_ID(3, __div32(65537ul, 3ul), 21845ul);
    return 0;
}
