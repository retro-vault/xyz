#include "xcc_exec_test.h"

static unsigned int read_word_slot(unsigned char idx) {
    unsigned int data[4];
    data[0] = 0x1111u;
    data[1] = 0x2222u;
    data[2] = 0x3333u;
    data[3] = 0x4444u;
    return data[idx];
}

static unsigned int fill_byte_slots(void) {
    unsigned char lut[8];
    unsigned char i;
    for (i = 0u; i < 8u; ++i)
        lut[i] = (unsigned char)(i + 3u);
    return (unsigned int)lut[5] * 256u + (unsigned int)lut[7];
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, read_word_slot(0u), 0x1111u);
    XCC_CHECK_EQ_UINT_ID(2, read_word_slot(2u), 0x3333u);
    XCC_CHECK_EQ_UINT_ID(3, read_word_slot(3u), 0x4444u);
    XCC_CHECK_EQ_UINT_ID(4, fill_byte_slots(), 0x080Au);
    return 0;
}
