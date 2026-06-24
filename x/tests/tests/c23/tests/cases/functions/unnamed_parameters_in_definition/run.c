//
// Function definitions may omit parameter names.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int constant_sum(int, int)
{
    return 23;
}

int main(void)
{
    if (constant_sum(7, 8) != 23)
        return 1;

    puts("OK unnamed_parameters_in_definition");
    return 0;
}

