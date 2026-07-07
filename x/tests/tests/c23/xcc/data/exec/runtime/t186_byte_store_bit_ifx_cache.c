#include "xcc_exec_test.h"

static unsigned char crc_step(unsigned char crc, unsigned char byte) {
    crc ^= byte;
    if (crc & 0x80u)
        return (unsigned char)((crc << 1) ^ 0x07u);
    return (unsigned char)(crc << 1);
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, crc_step(0x01u, 0x80u), 0x05u);
    XCC_CHECK_EQ_UINT_ID(2, crc_step(0x80u, 0x80u), 0x00u);
    return 0;
}
