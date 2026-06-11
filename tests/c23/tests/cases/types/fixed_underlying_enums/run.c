//
// Enumerations may specify a fixed underlying type with colon syntax.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


enum tiny : unsigned char
{
    tiny_constant = 200
};

int main(void)
{
    static_assert(_Generic((enum tiny)tiny_constant, unsigned char: 1, default: 0));
    puts("OK fixed_underlying_enums");
    return 0;
}

