#include "xcc_exec_test.h"

#include <stddef.h>
#include <stdlib.h>

static int cmp_int(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int main(void) {
    int values[6];
    int key;
    int *hit;

    values[0] = 5;
    values[1] = 1;
    values[2] = 4;
    values[3] = 2;
    values[4] = 6;
    values[5] = 3;

    qsort(values, 6u, sizeof(values[0]), cmp_int);
    if (values[0] != 1) return 1;
    if (values[1] != 2) return 2;
    if (values[2] != 3) return 3;
    if (values[3] != 4) return 4;
    if (values[4] != 5) return 5;
    if (values[5] != 6) return 6;

    key = 4;
    hit = (int *)bsearch(&key, values, 6u, sizeof(values[0]), cmp_int);
    if (hit == (void *)0) return 7;
    if (*hit != 4) return 8;

    key = 7;
    hit = (int *)bsearch(&key, values, 6u, sizeof(values[0]), cmp_int);
    if (hit != (void *)0) return 9;

    return 0;
}
