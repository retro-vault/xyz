#include "xcc_exec_test.h"

#include <stdlib.h>

int main(void) {
    int *items = (int *)0;
    int *probe;
    int sizes[] = { 1, 2, 4, 8, 16 };
    int i;
    int j;

    for (i = 0; i < 5; ++i) {
        int *next = (int *)realloc(items, (size_t)sizes[i] * sizeof(int));
        XCC_CHECK_ID(1, next != (int *)0);
        items = next;
        for (j = 0; j < sizes[i]; ++j) {
            items[j] = j;
        }
    }

    probe = (int *)malloc(2u * sizeof(int));
    XCC_CHECK_ID(2, probe != (int *)0);
    for (i = 0; i < 16; ++i) {
        XCC_CHECK_ID(3, items + i != probe);
    }

    probe[0] = -4;
    probe[1] = 42;
    XCC_CHECK_EQ_INT_ID(4, items[0], 0);
    XCC_CHECK_EQ_INT_ID(5, items[15], 15);

    free(probe);
    free(items);
    return 0;
}
