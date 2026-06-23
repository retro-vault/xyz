#include "xcc_exec_test.h"

static unsigned char classify(unsigned char value) {
    switch (value & 7u) {
    case 0u: return (unsigned char)(value ^ 0x10u);
    case 1u: return (unsigned char)(value ^ 0x21u);
    case 2u: return (unsigned char)(value ^ 0x32u);
    case 3u: return (unsigned char)(value ^ 0x43u);
    case 4u: return (unsigned char)(value ^ 0x54u);
    case 5u: return (unsigned char)(value ^ 0x65u);
    case 6u: return (unsigned char)(value ^ 0x76u);
    default: return (unsigned char)(value ^ 0x07u);
    }
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, classify(0u),   0x10u);
    XCC_CHECK_EQ_UINT_ID(2, classify(1u),   0x20u);
    XCC_CHECK_EQ_UINT_ID(3, classify(2u),   0x30u);
    XCC_CHECK_EQ_UINT_ID(4, classify(3u),   0x40u);
    XCC_CHECK_EQ_UINT_ID(5, classify(4u),   0x50u);
    XCC_CHECK_EQ_UINT_ID(6, classify(5u),   0x60u);
    XCC_CHECK_EQ_UINT_ID(7, classify(6u),   0x70u);
    XCC_CHECK_EQ_UINT_ID(8, classify(7u),   0x00u);
    XCC_CHECK_EQ_UINT_ID(9, classify(254u), 0x88u);
    XCC_CHECK_EQ_UINT_ID(10, classify(255u), 0xF8u);
    return 0;
}
