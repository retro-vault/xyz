//
// Compound literals accept storage-class specifiers such as static.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


static int *sample(void)
{
    return (static int[2]){1, 2};
}

int main(void)
{
    int *first = sample();
    int *second = sample();

    if (first != second)
        return 1;

    if (first[0] != 1 || first[1] != 2)
        return 1;

    first[0] = 9;
    if (sample()[0] != 9)
        return 1;

    puts("OK compound_literal_storage_class");
    return 0;
}

