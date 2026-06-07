#include "xcc_exec_test.h"

static unsigned int crc_like_step(unsigned int x, unsigned char rounds) {
    unsigned char i = 0u;
    while (i < rounds) {
        if (x & 0x8000u) {
            x = (unsigned int)((x << 1) ^ 0x1021u);
        } else {
            x = (unsigned int)(x << 1);
        }
        ++i;
    }
    return x;
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, crc_like_step(0x1234u, 8u), 1651u);
    XCC_CHECK_EQ_UINT_ID(2, crc_like_step(0x8001u, 4u), 33048u);
    XCC_CHECK_EQ_UINT_ID(3, crc_like_step(0x4000u, 1u), 32768u);
    return 0;
}
