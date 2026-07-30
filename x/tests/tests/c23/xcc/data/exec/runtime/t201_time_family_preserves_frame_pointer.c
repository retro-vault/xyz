#include <stdint.h>
#include <string.h>
#include <time.h>

static int check_time_frames(void) {
    volatile uint16_t before = 0x1357u;
    volatile uint16_t after = 0x2468u;
    time_t stamp = (time_t)-1;
    char text[26];

    if (time(&stamp) != stamp)
        return 1;
    if (ctime_r(&stamp, text) != text)
        return 2;
    if (strlen(text) != 25u || text[24] != '\n')
        return 3;
    if (before != 0x1357u || after != 0x2468u)
        return 4;
    return 0;
}

int main(void) {
    volatile uint16_t *mailbox = (volatile uint16_t *)0xff00u;
    volatile unsigned char *done = (volatile unsigned char *)0xff02u;

    *mailbox = (uint16_t)check_time_frames();
    *done = 0xa5u;
    for (;;) {
    }
}
