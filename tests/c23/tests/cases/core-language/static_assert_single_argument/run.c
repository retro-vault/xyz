//
// The one-argument static_assert form is accepted.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


static_assert(sizeof(int) >= 2);

int main(void)
{
    puts("OK static_assert_single_argument");
    return 0;
}

