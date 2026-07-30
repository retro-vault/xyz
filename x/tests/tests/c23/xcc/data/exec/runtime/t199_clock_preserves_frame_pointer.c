#include <stdint.h>
#include <time.h>

static int check_clock_frame(void) {
    volatile uint16_t before = 0x1357u;
    volatile uint16_t after = 0x2468u;
    clock_t sample = clock();

    return before == 0x1357u && after == 0x2468u && sample >= (clock_t)0;
}

int main(void) {
    volatile uint16_t *mailbox = (volatile uint16_t *)0xff00u;
    volatile unsigned char *done = (volatile unsigned char *)0xff02u;

    *mailbox = check_clock_frame() ? 0u : 1u;
    *done = 0xa5u;
    for (;;) {
    }
}
