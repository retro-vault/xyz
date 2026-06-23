#include "xcc_exec_test.h"

#include <stdlib.h>

static void handler0(void) {}
static void handler1(void) {}
static void handler2(void) {}
static void handler3(void) {}
static void handler4(void) {}
static void handler5(void) {}
static void handler6(void) {}
static void handler7(void) {}
static void handler8(void) {}

int main(void) {
    ldiv_t qr;
    lldiv_t qr64;

    qr = ldiv(-100L, 7L);
    if (qr.quot != -14L) return 1;
    if (qr.rem != -2L) return 2;

    qr = ldiv(100L, -9L);
    if (qr.quot != -11L) return 3;
    if (qr.rem != 1L) return 4;

    qr64 = lldiv(-1000000000000LL, 97LL);
    if (qr64.quot != -10309278350LL) return 5;
    if (qr64.rem != -50LL) return 6;

    if (atexit(handler0) != 0) return 7;
    if (atexit(handler1) != 0) return 8;
    if (atexit(handler2) != 0) return 9;
    if (atexit(handler3) != 0) return 10;
    if (atexit(handler4) != 0) return 11;
    if (atexit(handler5) != 0) return 12;
    if (atexit(handler6) != 0) return 13;
    if (atexit(handler7) != 0) return 14;
    if (atexit(handler8) == 0) return 15;

    if (at_quick_exit(handler0) != 0) return 16;
    if (at_quick_exit(handler1) != 0) return 17;
    if (at_quick_exit(handler2) != 0) return 18;
    if (at_quick_exit(handler3) != 0) return 19;
    if (at_quick_exit(handler4) != 0) return 20;
    if (at_quick_exit(handler5) != 0) return 21;
    if (at_quick_exit(handler6) != 0) return 22;
    if (at_quick_exit(handler7) != 0) return 23;
    if (at_quick_exit(handler8) == 0) return 24;

    return 0;
}
