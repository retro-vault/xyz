//
// The [[fallthrough]] attribute is accepted in switch statements.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    int value = 1;
    int total = 0;

    switch (value) {
    case 1:
        total += 20;
        [[fallthrough]];
    case 2:
        total += 3;
        break;
    default:
        return 1;
    }

    if (total != 23)
        return 1;

    puts("OK attr_fallthrough");
    return 0;
}

