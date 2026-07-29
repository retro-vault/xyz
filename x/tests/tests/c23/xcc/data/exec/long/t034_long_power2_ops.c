#include "xcc_exec_test.h"

extern unsigned long _mul32(unsigned long a, unsigned long b);
extern unsigned long _div32(unsigned long a, unsigned long b);
extern unsigned long _mod32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_ULONG_ID(1, _div32(123456ul, 8ul), 15432ul);
    XCC_CHECK_EQ_ULONG_ID(2, _mod32(123456ul, 32ul), 0ul);
    XCC_CHECK_EQ_ULONG_ID(3, _mul32(123456ul, 4ul), 493824ul);
    return 0;
}
