//
// The enhanced auto specifier infers object types from initializers.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    auto whole = 23;
    auto fraction = 1.5;

    static_assert(_Generic(whole, int: 1, default: 0));
    static_assert(_Generic(fraction, double: 1, default: 0));

    if (whole != 23)
        return 1;

    if (fraction <= 1.4 || fraction >= 1.6)
        return 1;

    puts("OK auto_type_inference");
    return 0;
}

