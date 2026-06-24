//
// The wb and uwb literal suffixes produce bit-precise integer constants.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    static_assert(_Generic(3wb, _BitInt(3): 1, default: 0));
    static_assert(_Generic(3uwb, unsigned _BitInt(2): 1, default: 0));

    if ((_BitInt(3))3wb != 3)
        return 1;

    if ((unsigned _BitInt(2))3uwb != 3u)
        return 1;

    puts("OK bitint_literal_suffixes");
    return 0;
}

