//
// __has_c_attribute exposes the standard attribute revisions.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#if __has_c_attribute(maybe_unused) < 202106L
#error maybe_unused should be visible through __has_c_attribute.
#endif

#if __has_c_attribute(noreturn) < 202202L
#error noreturn should be visible through __has_c_attribute.
#endif

int main(void)
{
    puts("OK has_c_attribute_operator");
    return 0;
}

