//
// Objects can be initialized with empty braces.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


struct pair
{
    int left;
    int right;
};

int main(void)
{
    int values[3] = {};
    struct pair p = {};

    if (values[0] != 0 || values[1] != 0 || values[2] != 0)
        return 1;

    if (p.left != 0 || p.right != 0)
        return 1;

    puts("OK empty_initializer");
    return 0;
}

