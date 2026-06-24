#include "xcc_exec_test.h"

int main(void) {
    unsigned long a = 0x0000fffful;
    unsigned long b = 0x12345678ul;

    XCC_CHECK_EQ_U32_ID(1, a + 1ul, 0x0000u, 0x0001u);
    XCC_CHECK_EQ_U32_ID(2, b - 0x00005678ul, 0x0000u, 0x1234u);
    XCC_CHECK_EQ_U32_ID(3, 0ul - 0ul, 0x0000u, 0x0000u);
    return 0;
}
