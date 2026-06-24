//
// The __has_include conditional inclusion operator works in preprocessing expressions.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#if !__has_include(<stdio.h>)
#error __has_include should detect stdio.h.
#endif

#if __has_include(<this_header_should_not_exist_anywhere.h>)
#error __has_include must report a missing header as unavailable.
#endif

int main(void)
{
    puts("OK has_include");
    return 0;
}

