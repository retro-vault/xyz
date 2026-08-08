#include "xcc_exec_test.h"

static int
iterative_search(const int *values, int left, int right, int wanted)
{
    int middle = left + (right - left) / 2;

    while (values[middle] != wanted) {
        if (right <= left || right < 0)
            return -1;
        if (values[middle] > wanted)
            right = middle - 1;
        else
            left = middle + 1;
        middle = left + (right - left) / 2;
    }
    return middle;
}

__attribute__((noinline)) int
xcc_scan_length(const int *position, int start)
{
    return *position - start;
}

__attribute__((noinline)) int
xcc_scan_to_comma(int *position, const char *text, int length, void *tokens)
{
    int start = *position;

    while (*position < length && text[*position] != ',' &&
           text[*position] != '\0')
        ++*position;
    if (tokens == 0)
        return xcc_scan_length(position, start);
    return -1;
}

int
main(void)
{
    static const int values[7] = {1, 4, 7, 10, 13, 16, 19};
    int position = 0;

    XCC_CHECK_EQ_INT_ID(1, iterative_search(values, 0, 6, 10), 3);
    XCC_CHECK_EQ_INT_ID(2, iterative_search(values, 0, 6, 19), 6);
    XCC_CHECK_EQ_INT_ID(3, iterative_search(values, 0, 6, 8), -1);
    XCC_CHECK_EQ_INT_ID(4, xcc_scan_to_comma(&position, "alpha,beta", 10, 0), 5);
    XCC_CHECK_EQ_INT_ID(5, position, 5);
    return 0;
}
