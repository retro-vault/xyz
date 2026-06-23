//
// The [[nodiscard]] attribute is accepted on functions.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


[[nodiscard]] static int compute_value(void)
{
    return 23;
}

int main(void)
{
    if (compute_value() != 23)
        return 1;

    puts("OK attr_nodiscard");
    return 0;
}

