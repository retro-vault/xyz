#include "xcc_exec_test.h"

static unsigned int state;

unsigned int
set_state(unsigned int value)
{
    state = value;
    return value + 1u;
}

unsigned int
bump_state(void)
{
    state = state + 1u;
    return state;
}

int
edge_failure(void)
{
    return 4;
}

int
main(void)
{
    unsigned int total = 0u;
    unsigned int snapshot;

    (void)set_state(37u);
    total = total + state;
    if (total != 37u)
        return 1;

    snapshot = state;
    (void)bump_state();
    if (snapshot != 37u)
        return 2;
    if (state != 38u)
        return 3;
    if (state != 38u)
        return edge_failure();
    return 0;
}
