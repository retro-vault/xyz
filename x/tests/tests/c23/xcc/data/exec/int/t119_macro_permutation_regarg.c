#include "xcc_exec_test.h"

#define PERMUTE(a, b, c, d) \
    (((a) << 12) | ((b) << 8) | ((c) << 4) | (d))

static int
compare_walk(const char *a, const char *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return (int)((unsigned char)*a - (unsigned char)*b);
}

int
main(void)
{
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;

    /* Formal substitutions are simultaneous: the actual identifiers must
       not be substituted again as later formal parameters are processed. */
    XCC_CHECK_EQ_INT_ID(1, PERMUTE(d, a, b, c), 0x4123);

    /* Two register-passed pointers also exercise prologue materialization:
       moving one into IY must not overwrite the other's frame spill. */
    XCC_CHECK_EQ_INT_ID(2, compare_walk("same-x", "same-y"), -1);
    XCC_CHECK_EQ_INT_ID(3, compare_walk("same", "same"), 0);
    return 0;
}
