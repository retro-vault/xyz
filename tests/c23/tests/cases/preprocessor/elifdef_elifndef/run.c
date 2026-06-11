//
// The #elifdef and #elifndef directives work as conditional branches.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#define FEATURE_PRESENT 1

#if 0
#error The first branch should not be selected.
#elifdef FEATURE_PRESENT
#define BRANCH_VALUE 23
#elifndef FEATURE_PRESENT
#define BRANCH_VALUE -1
#else
#define BRANCH_VALUE -2
#endif

int main(void)
{
    if (BRANCH_VALUE != 23)
        return 1;

    puts("OK elifdef_elifndef");
    return 0;
}

