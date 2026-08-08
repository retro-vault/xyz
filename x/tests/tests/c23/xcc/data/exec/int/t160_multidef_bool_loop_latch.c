#include "xcc_exec_test.h"

static void
bubble_sort_five(int *values)
{
    _Bool sorted = 0;

    while (!sorted) {
        int i;

        sorted = 1;
        for (i = 0; i < 4; ++i) {
            if (values[i] > values[i + 1]) {
                int temporary = values[i];
                values[i] = values[i + 1];
                values[i + 1] = temporary;
                sorted = 0;
            }
        }
    }
}

int
main(void)
{
    int values[5] = {5, 1, 4, 2, 3};
    unsigned int i;

    bubble_sort_five(values);
    for (i = 0; i < 5; ++i)
        XCC_CHECK_EQ_INT_ID(i + 1, values[i], (int)i + 1);
    return 0;
}
