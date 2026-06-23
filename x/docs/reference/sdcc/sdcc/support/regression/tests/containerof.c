/*
 * test case for containerof
 */

#include <testfwk.h>

#ifdef __SDCC
/* the following definitions have been borrowed from stddef.h */
#define offsetof(s, m) __builtin_offsetof (s, m)
#define containerof(p, t, m) ((t *)((char *)(p) - offsetof(t, m)))

typedef struct
{
    int k;
    int l;
    int m;
} t;
#endif

void
testContainerOf(void)
{
#ifdef __SDCC
    t o;
    int *p = &o.m;
    t *po = containerof(p, t, m);
    ASSERT(po == &o);
#endif
}
