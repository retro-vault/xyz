#include "xcc_exec_test.h"

struct sample {
    unsigned int prefix;
    unsigned int value;
    unsigned int counter;
    unsigned char gate;
};

static __attribute__((noinline)) unsigned int
masked_update(struct sample *item)
{
    unsigned int selected = item->value & 0x0fffu;

    if (item->gate)
        item->counter++;
    else
        item->counter--;
    return selected + item->counter;
}

int
main(void)
{
    struct sample up = { 0x1111u, 0xabcdu, 0x1200u, 1u };
    struct sample down = { 0x2222u, 0xf234u, 0x0100u, 0u };

    XCC_CHECK_EQ_UINT_ID(1, masked_update(&up), 0x1dceu);
    XCC_CHECK_EQ_UINT_ID(2, up.counter, 0x1201u);
    XCC_CHECK_EQ_UINT_ID(3, masked_update(&down), 0x0333u);
    XCC_CHECK_EQ_UINT_ID(4, down.counter, 0x00ffu);
    return 0;
}
