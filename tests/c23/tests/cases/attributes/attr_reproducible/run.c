//
// The [[reproducible]] attribute is accepted on function declarators.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


static int twice(int x) [[reproducible]];
static int twice(int x)
{
    return x * 2;
}

int main(void)
{
    if (twice(11) != 22)
        return 1;

    puts("OK attr_reproducible");
    return 0;
}

