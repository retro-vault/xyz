#include "xcc_exec_test.h"

#include <inttypes.h>
#include <wchar.h>

static void init_ws(wchar_t *dst, const char *src) {
    while (*src) {
        *dst++ = (unsigned char)*src++;
    }
    *dst = 0;
}

int main(void) {
    wchar_t ws_dec[7];
    wchar_t ws_hex[6];
    wchar_t ws_umax[5];
    wchar_t *wend;
    char *end;
    long lv;
    unsigned long ul;
    intmax_t imv;
    uintmax_t umv;
    imaxdiv_t qr;

    init_ws(ws_dec, " -123x");
    init_ws(ws_hex, "0xffz");
    init_ws(ws_umax, "ffff");

    lv = wcstol(ws_dec, &wend, 0);
    if (lv != -123L) return 1;
    if (wend != &ws_dec[5]) return 2;

    ul = wcstoul(ws_hex, &wend, 0);
    if (ul != 0xffUL) return 3;
    if (wend != &ws_hex[4]) return 4;

    umv = wcstoumax(ws_umax, &wend, 16);
    XCC_CHECK_EQ_U64_ID(5, umv, 0xffff, 0x0000, 0x0000, 0x0000);
    if (wend != &ws_umax[4]) return 6;

    qr = imaxdiv(-17, 5);
    XCC_CHECK_EQ_S64_ID(7, qr.quot, 0xfffd, 0xffff, 0xffff, 0xffff);
    XCC_CHECK_EQ_S64_ID(8, qr.rem, 0xfffe, 0xffff, 0xffff, 0xffff);

    imv = strtoimax("7fff", &end, 16);
    XCC_CHECK_EQ_S64_ID(9, imv, 0x7fff, 0x0000, 0x0000, 0x0000);
    if (*end != '\0') return 10;

    umv = strtoumax("177tail", &end, 8);
    XCC_CHECK_EQ_U64_ID(11, umv, 0x007f, 0x0000, 0x0000, 0x0000);
    if (*end != 't') return 12;

    return 0;
}
