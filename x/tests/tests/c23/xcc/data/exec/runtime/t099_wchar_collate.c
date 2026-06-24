#include "xcc_exec_test.h"

#include <wchar.h>

static void init_ws(wchar_t *dst, const char *src) {
    while (*src) {
        *dst++ = (unsigned char)*src++;
    }
    *dst = 0;
}

static int wstreq(const wchar_t *a, const wchar_t *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

int main(void) {
    wchar_t ws_word[5];
    wchar_t ws_abc[4];
    wchar_t ws_abd[4];
    wchar_t xbuf[8];
    size_t xlen;

    init_ws(ws_word, "test");
    init_ws(ws_abc, "abc");
    init_ws(ws_abd, "abd");

    xlen = wcsxfrm(xbuf, ws_word, 8u);
    if (xlen != 4u) return 1;
    if (!wstreq(xbuf, ws_word)) return 2;

    xbuf[0] = 0xffffu;
    xbuf[1] = 0xffffu;
    xlen = wcsxfrm(xbuf, ws_word, 1u);
    if (xlen != 4u) return 3;
    if (xbuf[0] != 0) return 4;

    if (wcscoll(ws_abc, ws_abd) >= 0) return 5;
    if (wcscoll(ws_abd, ws_abc) <= 0) return 6;
    if (wcscoll(ws_word, ws_word) != 0) return 7;

    return 0;
}
