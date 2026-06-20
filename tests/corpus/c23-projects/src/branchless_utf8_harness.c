#include <stdint.h>

#include "utf8.h"

int main(void) {
    unsigned char text[] = {
        'A', 0xc2, 0xa3, 0xe2, 0x82, 0xac, 0xf0, 0x9f,
        0x98, 0x80, 0, 0, 0, 0
    };
    unsigned char bad[] = {0xe2, 0x28, 0xa1, 0, 0, 0, 0};
    void *p = text;
    uint32_t cp = 0;
    int err = 0;

    p = utf8_decode(p, &cp, &err);
    if (err || cp != 'A' || p != text + 1) return 1;

    p = utf8_decode(p, &cp, &err);
    if (err || cp != 0x00a3 || p != text + 3) return 2;

    p = utf8_decode(p, &cp, &err);
    if (err || cp != 0x20ac || p != text + 6) return 3;

    p = utf8_decode(p, &cp, &err);
    if (err || cp != 0x1f600 || p != text + 10) return 4;

    p = utf8_decode(bad, &cp, &err);
    if (!err || p == bad) return 5;
    return 0;
}
