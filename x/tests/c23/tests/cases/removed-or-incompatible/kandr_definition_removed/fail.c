//
// Identifier-list function definitions are no longer supported.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


int sum(a, b)
int a;
int b;
{
    return a + b;
}

int main(void)
{
    printf("%d\n", sum(20, 3));
    return 0;
}

