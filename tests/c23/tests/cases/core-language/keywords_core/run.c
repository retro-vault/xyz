//
// Promoted keywords such as bool, thread_local, alignas, and alignof work as core syntax.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


alignas(32) thread_local int tls_value = 7;
static_assert(alignof(tls_value) >= 32);

int main(void)
{
    bool enabled = true;

    if (!enabled || false)
        return 1;

    if (tls_value != 7)
        return 1;

    puts("OK keywords_core");
    return 0;
}

