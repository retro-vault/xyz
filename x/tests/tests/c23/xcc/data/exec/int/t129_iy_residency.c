#include "xcc_exec_test.h"

struct mixer_state {
    unsigned int first;
    unsigned int second;
    unsigned int third;
    unsigned int fourth;
    unsigned int fifth;
};

static __attribute__((noinline)) unsigned int
mix_state(struct mixer_state *state, unsigned int rounds)
{
    unsigned int sum = 0;

    while (rounds != 0) {
        state->first = (state->first + state->second) & 0x7fffu;
        state->third ^= state->first;
        if ((state->third & 1u) != 0)
            state->fourth = (state->fourth + state->fifth) & 0x7fffu;
        else
            state->fifth = (state->fifth + state->first) & 0x7fffu;
        state->second = (state->second + state->third) & 0x7fffu;
        sum = (sum + state->first + state->fourth) & 0xffffu;
        --rounds;
    }
    return sum;
}

static __attribute__((noinline)) unsigned int
text_length(const char *text)
{
    const char *cursor = text;

    while (*cursor != '\0')
        ++cursor;
    return (unsigned int)(cursor - text);
}

static __attribute__((noinline)) unsigned int
total_text_length(const char *text, unsigned int count)
{
    unsigned int total = 0;

    while (count-- != 0) {
        unsigned int length = text_length(text);
        total += length;
        text += length + 1;
    }
    return total;
}

int
main(void)
{
    static const char texts[] = "a\0bbb\0cc\0hello\0";
    struct mixer_state state = { 1, 4, 7, 2, 9 };

    XCC_CHECK_EQ_UINT_ID(1, mix_state(&state, 19), 8922u);
    XCC_CHECK_EQ_UINT_ID(2, state.first, 20101u);
    XCC_CHECK_EQ_UINT_ID(3, state.second, 13371u);
    XCC_CHECK_EQ_UINT_ID(4, state.third, 23178u);
    XCC_CHECK_EQ_UINT_ID(5, state.fourth, 32566u);
    XCC_CHECK_EQ_UINT_ID(6, state.fifth, 21319u);
    XCC_CHECK_EQ_UINT_ID(7, total_text_length(texts, 4), 11u);
    return 0;
}
