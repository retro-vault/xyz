//
// Normal enumerations can represent values outside the old int-only range.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


enum huge_value
{
    huge_constant = 1ULL << 40
};

int main(void)
{
    enum huge_value value = huge_constant;

    if ((unsigned long long)value != (1ULL << 40))
        return 1;

    puts("OK enhanced_enums_large_values");
    return 0;
}

