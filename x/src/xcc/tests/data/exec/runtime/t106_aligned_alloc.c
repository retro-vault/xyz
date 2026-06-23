#include "xcc_exec_test.h"

#include <stdlib.h>

static unsigned char *p;
static unsigned char *q;
static unsigned char *r;
static unsigned int i;

int main(void) {
    if (aligned_alloc(0u, 8u) != (void *)0) return 1;
    if (aligned_alloc(3u, 12u) != (void *)0) return 2;
    if (aligned_alloc(8u, 12u) != (void *)0) return 3;

    p = (unsigned char *)aligned_alloc(8u, 16u);
    if (!p) return 4;
    if (((unsigned int)(unsigned long)p & 7u) != 0u) return 5;
    for (i = 0; i < 16u; ++i) p[i] = (unsigned char)(i + 1u);

    q = (unsigned char *)aligned_alloc(32u, 64u);
    if (!q) return 6;
    if (((unsigned int)(unsigned long)q & 31u) != 0u) return 7;

    r = (unsigned char *)realloc(p, 24u);
    if (!r) return 8;
    for (i = 0; i < 16u; ++i)
        if (r[i] != (unsigned char)(i + 1u)) return 9;

    free(q);
    free(r);
    return 0;
}
