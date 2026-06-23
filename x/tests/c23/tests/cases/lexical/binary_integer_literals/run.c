//
// Binary integer literals with 0b and 0B prefixes are accepted.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    unsigned int value = 0b10100101u;

    if (value != 165u)
        return 1;

    puts("OK binary_integer_literals");
    return 0;
}

