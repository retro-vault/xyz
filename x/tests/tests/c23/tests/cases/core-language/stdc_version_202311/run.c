//
// __STDC_VERSION__ reports a C23 language mode.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#error This compiler is not in a C23 language mode.
#endif

int main(void)
{
    puts("OK stdc_version_202311");
    return 0;
}

