#include "xcc_exec_test.h"

static volatile int first_value = 7;
static volatile int second_value = 8;

int
read_first_value(void)
{
    return first_value;
}

int
read_second_value(void)
{
    return second_value;
}

int
main(void)
{
    int value = read_first_value();

    if (value != 7)
        return 1;
    value = read_second_value();
    if (value != 7)
        return 0;
    return 2;
}
