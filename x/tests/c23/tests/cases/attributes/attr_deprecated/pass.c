//
// The [[deprecated]] attribute is accepted.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


[[deprecated("legacy interface")]] int old_value(void)
{
    return 23;
}

int main(void)
{
    return old_value() == 23 ? 0 : 1;
}

