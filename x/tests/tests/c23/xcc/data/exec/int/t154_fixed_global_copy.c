#include "xcc_exec_test.h"

static char output[16];
static const char first[] = "first";
static const char second[] = "second";

static __attribute__((noinline)) void
copy_text(char *destination, const char *source)
{
    while ((*destination++ = *source++))
        ;
}

int
main(void)
{
    copy_text(output, first);
    XCC_CHECK_EQ_INT_ID(1, output[0], 'f');
    XCC_CHECK_EQ_INT_ID(2, output[4], 't');
    XCC_CHECK_EQ_INT_ID(3, output[5], 0);

    copy_text(output, second);
    XCC_CHECK_EQ_INT_ID(4, output[0], 's');
    XCC_CHECK_EQ_INT_ID(5, output[5], 'd');
    XCC_CHECK_EQ_INT_ID(6, output[6], 0);
    return 0;
}
