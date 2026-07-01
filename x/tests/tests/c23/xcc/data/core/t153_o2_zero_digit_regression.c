#include <stdint.h>
#include <stdio.h>

static void fmt(uint32_t value, char *dst) {
    static const uint32_t powers[10] = {
        1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
        10000u, 1000u, 100u, 10u, 1u
    };
    uint8_t started = 0;

    for (uint8_t i = 0; i != 10; ++i) {
        uint8_t digit = 0;
        while (value >= powers[i]) {
            value -= powers[i];
            digit++;
        }
        if (digit || started || i == 9) {
            *dst++ = (char)('0' + digit);
            started = 1;
        }
    }

    *dst = 0;
}

int main(void) {
    static const uint32_t values[3] = {2304u, 10752u, 10240u};
    char buf[16];

    for (int i = 0; i < 3; ++i) {
        fmt(values[i], buf);
        puts(buf);
    }

    return 0;
}
