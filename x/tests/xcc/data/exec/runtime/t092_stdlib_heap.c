#include "xcc_exec_test.h"

#include <stddef.h>
#include <stdlib.h>

static unsigned char *a, *b, *c, *d, *e, *g;
static unsigned int i;

int main(void) {

    if (malloc(0) != (void *)0) return 1;
    if (calloc(0, 5) != (void *)0) return 2;
    if (calloc(0x8000u, 4u) != (void *)0) return 3;

    a = (unsigned char *)malloc(8u);
    b = (unsigned char *)calloc(4u, 3u);
    if (!a || !b) return 4;
    if (a == b) return 5;

    for (i = 0; i < 8u; ++i) a[i] = (unsigned char)(i + 1u);
    for (i = 0; i < 12u; ++i)
        if (b[i] != 0u) return 6;

    a = (unsigned char *)realloc(a, 16u);
    if (!a) return 7;
    for (i = 0; i < 8u; ++i)
        if (a[i] != (unsigned char)(i + 1u)) return 8;

    g = (unsigned char *)realloc((void *)0, 6u);
    if (!g) return 9;
    for (i = 0; i < 6u; ++i) g[i] = (unsigned char)(0xA0u + i);
    g = (unsigned char *)realloc(g, 0u);
    if (g != (void *)0) return 10;

    c = (unsigned char *)malloc(2000u);
    d = (unsigned char *)malloc(2000u);
    e = (unsigned char *)malloc(3500u);
    if (!c || !d || !e) return 11;
    free(c);
    free(d);
    c = (unsigned char *)malloc(3000u);
    if (!c) return 12;     /* requires free-list coalescing */

    free(c);
    free(e);
    free(b);
    free(a);
    free((void *)0);
    return 0;
}
