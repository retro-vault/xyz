//
// constexpr object definitions participate in constant-expression contexts.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


constexpr int answer = 42;
static_assert(answer == 42);
int table[answer == 42 ? 1 : -1];

int main(void)
{
    if ((int)(sizeof(table) / sizeof(table[0])) != 1)
        return 1;

    puts("OK constexpr_objects");
    return 0;
}

