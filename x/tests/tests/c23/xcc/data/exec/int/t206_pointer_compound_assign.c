#include "xcc_exec_test.h"

struct triple {
    int x;
    int y;
    int z;
};

static volatile unsigned int observed;

static __attribute__((noinline)) void
observe_cursor(const int *cursor, const char *label)
{
    observed = (unsigned int)(cursor[0] + label[0]);
}

int
main(void)
{
    int words[16] = {0};
    struct triple triples[5] = {{0}};
    char label[2] = "A";
    int *cursor = words;
    struct triple *triple_cursor = triples;
    unsigned int dynamic_step = 2;

    observe_cursor(cursor, label);
    label[0]++;
    cursor += 6;
    *cursor = 0x1234;

    if (label[0] != 'B')
        return 1;
    if (words[6] != 0x1234 || words[3] != 0)
        return 2;

    cursor -= 4;
    *cursor = 0x5678;
    if (words[2] != 0x5678)
        return 3;

    triple_cursor += dynamic_step;
    triple_cursor->y = 0x1357;
    if (triples[2].y != 0x1357 || triples[1].x != 0)
        return 4;

    triple_cursor -= dynamic_step;
    triple_cursor->z = 0x2468;
    if (triples[0].z != 0x2468)
        return 5;

    return observed == (unsigned int)'A' ? 0 : 6;
}
