//
// Function declarations with an empty parameter list behave like (void).
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int sample();
int sample()
{
    return 17;
}

static_assert(_Generic(&sample, int (*)(void): 1, default: 0));

int main(void)
{
    if (sample() != 17)
        return 1;

    puts("OK empty_parameter_list_is_void");
    return 0;
}

