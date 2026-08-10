#include <stddef.h>
#include <string.h>

#include "xcc_exec_test.h"

static size_t
find_top_level(const char *text, char key)
{
    int depth = 0;
    size_t i;

    for (i = 0; i < strlen(text); ++i) {
        const char current = text[i];

        if (depth == 0 && current == key)
            return i;
        if (current == '(')
            ++depth;
        if (current == ')')
            --depth;
    }
    return 0;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, find_top_level("c|a*\nb", '|'), 1u);
    XCC_CHECK_EQ_UINT_ID(2, find_top_level("(c|a*\nb)", '|'), 0u);
    XCC_CHECK_EQ_UINT_ID(3, find_top_level("ab|cd", '|'), 2u);
    return 0;
}
