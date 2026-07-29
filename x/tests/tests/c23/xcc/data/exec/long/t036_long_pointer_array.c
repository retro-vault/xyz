#include "xcc_exec_test.h"

extern unsigned long _div32(unsigned long a, unsigned long b);
extern unsigned long _mod32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_ULONG_ID(1, _div32(987654321ul, 9ul), 109739369ul);
    XCC_CHECK_EQ_ULONG_ID(2, _mod32(987654321ul, 9ul), 0ul);
    XCC_CHECK_EQ_ULONG_ID(3, _div32(65537ul, 3ul), 21845ul);
    return 0;
}
