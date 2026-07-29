#include "xcc_exec_test.h"

extern unsigned long _mul32(unsigned long a, unsigned long b);
extern unsigned long _div32(unsigned long a, unsigned long b);

int main(void) {
    unsigned long x = _mul32(70000ul, 9ul);
    unsigned long y = _div32(x, 7ul);

    XCC_CHECK_EQ_ULONG_ID(1, x, 630000ul);
    XCC_CHECK_EQ_ULONG_ID(2, y, 90000ul);
    XCC_CHECK_EQ_ULONG_ID(3, y + 1234ul, 91234ul);
    return 0;
}
