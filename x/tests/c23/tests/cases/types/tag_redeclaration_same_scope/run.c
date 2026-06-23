//
// A tagged type may be redeclared with the same content in the same scope.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


struct point
{
    int x;
    int y;
};

struct point
{
    int x;
    int y;
};

int main(void)
{
    struct point p = {2, 3};

    if (p.x + p.y != 5)
        return 1;

    puts("OK tag_redeclaration_same_scope");
    return 0;
}

