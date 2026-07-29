#include "xcc_exec_test.h"

struct registry {
    unsigned reserved;
    unsigned tags[4];
    unsigned values[4];
    unsigned count;
};

static struct registry entries;

__attribute__((noinline)) void
registry_append(unsigned tag, unsigned value)
{
    unsigned index = entries.count++;
    entries.tags[index] = tag;
    entries.values[index] = value;
}

int
main(void)
{
    registry_append(0x1234u, 0xabcdU);
    registry_append(0x5678u, 0xef01U);

    XCC_CHECK_EQ_UINT_ID(1, entries.count, 2u);
    XCC_CHECK_EQ_UINT_ID(2, entries.tags[0], 0x1234u);
    XCC_CHECK_EQ_UINT_ID(3, entries.values[0], 0xabcdU);
    XCC_CHECK_EQ_UINT_ID(4, entries.tags[1], 0x5678u);
    XCC_CHECK_EQ_UINT_ID(5, entries.values[1], 0xef01U);
    return 0;
}
