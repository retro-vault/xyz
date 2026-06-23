//
// Labels may appear before declarations and at the end of compound statements.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
start:
    int value = 23;
    if (value != 23)
        return 1;

    {
        goto done;
    done:
    }

    puts("OK labels_before_declarations");
    return 0;
}

