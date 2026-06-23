//
// typeof and typeof_unqual deduce qualified and unqualified forms as specified.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int main(void)
{
    const int value = 23;
    static_assert(_Generic((typeof(value) *)0, const int *: 1, default: 0));
    static_assert(_Generic((typeof_unqual(value) *)0, int *: 1, default: 0));

    puts("OK typeof_and_typeof_unqual");
    return 0;
}

