//
// Identifiers can use Unicode character names that conform to the C23 rules.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


static int \u03b1\u03b2\u03b3(void)
{
    return 23;
}

int main(void)
{
    if (\u03b1\u03b2\u03b3() != 23)
        return 1;

    puts("OK unicode_identifiers");
    return 0;
}

