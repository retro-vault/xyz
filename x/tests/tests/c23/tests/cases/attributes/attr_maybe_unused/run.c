//
// The [[maybe_unused]] attribute is accepted.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    [[maybe_unused]] int value = 23;

    puts("OK attr_maybe_unused");
    return 0;
}

