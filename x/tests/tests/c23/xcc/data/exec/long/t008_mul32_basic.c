#include "xcc_exec_test.h"

extern unsigned long _mul32(unsigned long a, unsigned long b);

int main(void) {
    XCC_CHECK_EQ_U32_ID(1, _mul32(70000ul, 3ul), 0x3450u, 0x0003u);
    XCC_CHECK_EQ_U32_ID(2, _mul32(0ul, 123456ul), 0x0000u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(3, _mul32(1234ul, 5678ul), 0xe9bcu, 0x006au);
    return 0;
}
