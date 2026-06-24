#include <stdio.h>

int g_sum = 0;

int main(void) {
    volatile int left = 12;
    volatile int right = 30;
    volatile int sum = left + right;

    g_sum = sum;
    putchar('4');
    putchar('2');
    putchar('\n');
    __asm__("halt");
    return 0;
}
