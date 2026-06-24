#include "exact-int.h"

int main(void) {
    psnip_uint16_t word = 0x1234;
    psnip_uint8_t byte = 0x56;

    return sizeof(word) == 2 && sizeof(byte) == 1 &&
           word == 0x1234 && byte == 0x56 ? 0 : 1;
}
