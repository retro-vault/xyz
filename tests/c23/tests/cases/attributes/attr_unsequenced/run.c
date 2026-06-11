//
// The [[unsequenced]] attribute is accepted on function declarators.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


static int add_one(int x) [[unsequenced]];
static int add_one(int x)
{
    return x + 1;
}

int main(void)
{
    if (add_one(22) != 23)
        return 1;

    puts("OK attr_unsequenced");
    return 0;
}

