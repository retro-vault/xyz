//
// Digit separators with the single quote character are accepted in integer literals.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    unsigned long long big = 1'000'000ull;
    unsigned int mask = 0b1010'0101u;

    if (big != 1000000ull)
        return 1;

    if (mask != 165u)
        return 1;

    puts("OK digit_separators");
    return 0;
}

