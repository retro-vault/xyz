#include "xcc_exec_test.h"

static unsigned char const payload[] = {3u, 5u, 9u, 17u, 33u, 65u};
static unsigned char const *table_pointer = payload;

unsigned int
lookup_global_pointer(unsigned char index)
{
    return table_pointer[index];
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, lookup_global_pointer(0u), 3u);
    XCC_CHECK_EQ_UINT_ID(2, lookup_global_pointer(3u), 17u);
    XCC_CHECK_EQ_UINT_ID(3, lookup_global_pointer(5u), 65u);
    return 0;
}
