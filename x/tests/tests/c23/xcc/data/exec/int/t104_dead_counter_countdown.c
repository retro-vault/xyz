#include "xcc_exec_test.h"

static volatile unsigned char countdown_hits;

void
countdown_tick(void)
{
    countdown_hits = (unsigned char)(countdown_hits + 1u);
}

unsigned char
run_dead_counter(void)
{
    unsigned int i;

    for (i = 0u; i < 17u; ++i)
        countdown_tick();
    return countdown_hits;
}

unsigned int
run_observable_counter(void)
{
    unsigned int i;
    unsigned int sum = 0u;

    for (i = 0u; i < 9u; ++i)
        sum += i;
    return sum + i;
}

unsigned char
run_reused_dead_counter(void)
{
    unsigned int i;

    countdown_hits = 0u;
    for (i = 0u; i < 3u; ++i)
        countdown_tick();
    for (i = 0u; i < 5u; ++i)
        countdown_tick();
    return countdown_hits;
}

int
main(void)
{
    countdown_hits = 0u;
    XCC_CHECK_EQ_UINT_ID(1, run_dead_counter(), 17u);
    XCC_CHECK_EQ_UINT_ID(2, run_observable_counter(), 45u);
    XCC_CHECK_EQ_UINT_ID(3, run_reused_dead_counter(), 8u);
    return 0;
}
