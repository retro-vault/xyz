#include <string.h>

volatile long t188_value = 8;

long t188_read_value(void) {
    return t188_value;
}

int main(void) {
    volatile unsigned char padding[230];
    unsigned char left[2];
    unsigned char right[2];
    long saved;

    padding[0] = 3;
    padding[229] = 5;
    left[0] = right[0] = 0x12;
    left[1] = right[1] = 0x34;

    if (memcmp(left, right, sizeof(left)) != 0)
        return 1;
    if (memcmp(left, right, 1) != 0)
        return 1;
    if (memcmp(left + 1, right + 1, 1) != 0)
        return 1;
    if (memcmp(left, right, sizeof(left)) != 0)
        return 1;

    if (t188_read_value() != 8)
        return 2;

    saved = t188_read_value();
    if (saved != 8)
        return 3;

    return padding[0] + padding[229] == 8 ? 0 : 4;
}
