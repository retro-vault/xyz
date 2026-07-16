#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned int
decode(const unsigned char *opcode)
{
    switch (*opcode) {
    case 0: return 11;
    case 1: return 23;
    case 2: return 37;
    case 3: return 41;
    case 4: return 59;
    case 5: return 61;
    default: return 0;
    }
}

int
main(void)
{
    static const unsigned char opcodes[] = { 0, 3, 5, 9 };

    XCC_CHECK_EQ_UINT_ID(1, decode(&opcodes[0]), 11u);
    XCC_CHECK_EQ_UINT_ID(2, decode(&opcodes[1]), 41u);
    XCC_CHECK_EQ_UINT_ID(3, decode(&opcodes[2]), 61u);
    XCC_CHECK_EQ_UINT_ID(4, decode(&opcodes[3]), 0u);
    return 0;
}
